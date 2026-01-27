use crate::system_network::{Param, SystemNetwork, TypeResult};
use alloc::format;
use alloc::{boxed::Box, string::String};
use async_trait::async_trait;
use core::convert::TryFrom;
use feature_frm::FeatureErrorCode::{FT_ERR_ARGS, FT_ERR_GENERAL};
use feature_frm::*;
use feature_macros::feature_instance;
use futures::channel::oneshot;
use futures::future::{self, Either};
use vdk::async_runtime::io::{AsyncSocket, SockAddr};
use vdk::async_runtime::runtime::{spawn, JoinHandle};
use vdk::log::{debug, error, info, warn};

// extern C functions
unsafe extern "C" {
    fn uv_netstatus_gettype(type_ptr: *mut u8) -> i32;
}

pub fn system_network_on_register(_name: &FeatureString) {
    debug!("system_network_on_register called");
}

pub fn system_network_on_create(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {
    debug!("system_network_on_create called");
}

pub fn system_network_on_required(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
    info!("system_network_on_required called");
}

pub fn system_network_on_detached(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
    info!("system_network_on_detached called");
}

pub fn system_network_on_destroy(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {
    debug!("system_network_on_destroy called");
}

pub fn system_network_on_unregister(_name: &FeatureString) {
    debug!("system_network_on_unregister called");
}

const AF_NETLINK: i32 = 16;
const SOCK_RAW: i32 = 3;
const NETLINK_ROUTE: i32 = 0;
const RTMGRP_LINK: u32 = 1;
const RTM_NEWLINK: u16 = 16;

pub struct SystemNetworkPrototype {
    pub proto: FeaturePrototype,
    pub str: String,
}

impl SystemNetworkPrototype {
    pub(crate) fn new(proto: FeaturePrototype) -> Self {
        SystemNetworkPrototype {
            proto,
            str: String::from("SystemNetworkImpl"),
        }
    }
}

#[feature_instance(name = "SystemNetwork")]
pub struct SystemNetworkImpl {
    instance: FeatureInstance,
    monitoring_stop: Option<oneshot::Sender<()>>,
    monitoring_handle: Option<JoinHandle<()>>,
}

impl SystemNetworkImpl {
    pub(crate) fn new(instance: FeatureInstance) -> Self {
        SystemNetworkImpl {
            instance,
            monitoring_stop: None,
            monitoring_handle: None,
        }
    }

    fn create_netlink_socket() -> Result<AsyncSocket, String> {
        let mut socket = AsyncSocket::new_with_protocol(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)
            .map_err(|e| format!("Failed to create netlink socket: {}", e))?;

        let addr = SockAddr::try_new_netlink(0, RTMGRP_LINK)
            .map_err(|e| format!("Failed to create netlink address: {}", e))?;

        socket
            .bind(addr)
            .map_err(|e| format!("Failed to bind netlink socket: {}", e))?;

        Ok(socket)
    }

    fn fetch_current_network_type() -> Result<NetType, String> {
        let mut type_value: u8 = 0;
        let result = unsafe { uv_netstatus_gettype(&mut type_value as *mut u8) };
        if result != 0 {
            return Err(format!("uv_netstatus_gettype failed: {}", result));
        }
        Self::network_type_from_value(type_value)
            .map_err(|e| format!("Unknown network type value: {}", e))
    }

    fn is_newlink_event(buf: &[u8]) -> bool {
        const NETLINK_HEADER_LEN: usize = 16;
        const NLMSG_TYPE_OFFSET: usize = 4;
        if buf.len() < NETLINK_HEADER_LEN {
            return false;
        }

        let type_bytes = [buf[NLMSG_TYPE_OFFSET], buf[NLMSG_TYPE_OFFSET + 1]];
        let nlmsg_type = u16::from_ne_bytes(type_bytes);

        nlmsg_type == RTM_NEWLINK
    }

    pub fn get_prototype(&self) -> Option<&SystemNetworkPrototype> {
        self.instance.get_prototype::<SystemNetworkPrototype>()
    }

    // Convert network type value to enum
    fn network_type_from_value(type_value: u8) -> Result<NetType, u8> {
        NetType::try_from(type_value)
    }

    fn try_stop_monitoring(&mut self) -> Option<JoinHandle<()>> {
        if let Some(stop_tx) = self.monitoring_stop.take() {
            if let Err(e) = stop_tx.send(()) {
                warn!("Monitoring send stop failed, err:{e:?}");
            }
        }
        self.monitoring_handle.take()
    }

    async fn wait_for_monitoring_task(handle: JoinHandle<()>) {
        info!("Waiting for monitoring task to exit...");
        match handle.await {
            Ok(_) => {
                info!("Monitoring task has exited successfully");
            }
            Err(e) => {
                warn!("Monitoring task was canceled: {:?}", e);
            }
        }
    }
}

#[async_trait]
impl SystemNetwork for SystemNetworkImpl {
    async fn get_type(&mut self) -> Result<TypeResult, PromiseError> {
        // Call C function to get network type (always fetch fresh)
        let mut type_value: u8 = 0;
        let result = unsafe { uv_netstatus_gettype(&mut type_value as *mut u8) };

        // Check the result
        if result != 0 {
            // Call failed, return error
            let error_msg = String::from("Failed to get network type");
            error!("SystemNetwork::get_type failed: {}", error_msg);
            return Err(PromiseError::new(FT_ERR_GENERAL as i32, error_msg));
        }

        // Convert type value to enum
        let net_type = Self::network_type_from_value(type_value).map_err(|e| {
            let error_msg = format!("Unknown network type value: {}", e);
            error!("SystemNetwork::get_type failed: {}", error_msg);
            PromiseError::new(FT_ERR_GENERAL as i32, error_msg)
        })?;
        let type_str = net_type.as_str();
        let network_type = FeatureString::new(type_str);

        // Create typeResult struct
        let mut type_result = TypeResult::new();
        type_result.set_type(network_type);

        info!("SystemNetwork::get_type success: {}", type_str);
        Ok(type_result)
    }

    async fn subscribe(&mut self, p: Option<Param>) -> Result<(), PromiseError> {
        let mut p = if let Some(p) = p {
            p
        } else {
            error!("system_network subscribe: param is none");
            return Err(PromiseError::new(FT_ERR_GENERAL as i32, "param is none"));
        };
        let cb = match p.take_callback() {
            Some(callback) => callback,
            None => {
                return Err(PromiseError::new(
                    FT_ERR_ARGS as i32,
                    String::from("Callback is null in param!"),
                ));
            }
        };

        let mut socket = match Self::create_netlink_socket() {
            Ok(sock) => sock,
            Err(e) => {
                return Err(PromiseError::new(FT_ERR_GENERAL as i32, e));
            }
        };

        // Stop any existing monitoring and wait for it to exit
        if let Some(old_handle) = self.try_stop_monitoring() {
            Self::wait_for_monitoring_task(old_handle).await;
        }

        let mut last_type: Option<NetType> = None;

        // Trigger initial callback with current network type
        match Self::fetch_current_network_type() {
            Ok(net_type) => {
                let type_str = net_type.as_str();
                let network_type = FeatureString::new(type_str);
                let mut type_result = TypeResult::new();
                type_result.set_type(network_type);
                cb.invoke(type_result);
                last_type = Some(net_type);
            }
            Err(e) => {
                error!("Failed to fetch current network type on subscribe: {}", e);
            }
        }

        let (stop_tx, stop_rx) = oneshot::channel();
        self.monitoring_stop = Some(stop_tx);

        let monitoring_future = async move {
            loop {
                let mut buf: [u8; 128] = [0; 128];
                match socket.recv(&mut buf).await {
                    Ok(nbytes) => {
                        if nbytes > 0 {
                            info!(
                                "*** Netlink socket readable - received {} bytes - network interface change detected ***",
                                nbytes
                            );
                            let payload = &buf[..nbytes as usize];
                            if SystemNetworkImpl::is_newlink_event(payload) {
                                match SystemNetworkImpl::fetch_current_network_type() {
                                    Ok(net_type) => {
                                        if last_type != Some(net_type) {
                                            let type_str = net_type.as_str();
                                            let network_type = FeatureString::new(type_str);
                                            let mut type_result = TypeResult::new();
                                            type_result.set_type(network_type);
                                            cb.invoke(type_result);
                                            last_type = Some(net_type);
                                        }
                                    }
                                    Err(e) => {
                                        error!("Failed to fetch current network type: {}", e);
                                    }
                                }
                            }
                        }
                    }
                    Err(e) => {
                        error!("Error reading from netlink socket: {}", e);
                        break;
                    }
                }
            }
        };

        let handle = spawn(async move {
            futures::pin_mut!(monitoring_future);
            match future::select(monitoring_future, stop_rx).await {
                Either::Left(_) => {
                    error!("Monitoring task terminated unexpectedly");
                }
                Either::Right((res, _)) => {
                    info!("Stop signal received, monitoring task exiting: {:?}", res);
                }
            }
        });

        self.monitoring_handle = Some(handle);

        info!("SystemNetwork::subscribe success");
        Ok(())
    }

    fn unsubscribe(&mut self) {
        let _handle = self.try_stop_monitoring();
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum NetType {
    Wifi,
    Bluetooth,
    None,
    Ethernet,
    Cellular,
    Tun,
}

impl TryFrom<u8> for NetType {
    type Error = u8;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(NetType::Wifi),
            2 => Ok(NetType::Bluetooth),
            3 => Ok(NetType::None),
            4 => Ok(NetType::Ethernet),
            5 => Ok(NetType::Cellular),
            6 => Ok(NetType::Tun),
            _ => Err(value),
        }
    }
}

impl NetType {
    fn as_str(&self) -> &'static str {
        match self {
            NetType::Wifi => "wifi",
            NetType::Bluetooth => "bluetooth",
            NetType::None => "none",
            NetType::Ethernet => "ethernet",
            NetType::Cellular => "cellular",
            NetType::Tun => "tun",
        }
    }
}
