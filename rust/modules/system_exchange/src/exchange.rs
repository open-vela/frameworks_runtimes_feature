use alloc::boxed::Box;
use async_trait::async_trait;
use core::ffi::CStr;
use core::ptr;
use feature_frm::*;
use feature_macros::feature_struct;
use libc::c_void;
use vdk::async_runtime::runtime;

use crate::exchange_impl::{
    system_exchange_on_create, system_exchange_on_destroy, system_exchange_on_detached,
    system_exchange_on_register, system_exchange_on_required, system_exchange_on_unregister,
    ExchangeImpl, ExchangePrototype,
};

unsafe extern "C" {
    pub(crate) fn system_exchange_ClearInfo_struct_get_type() -> FeatureType;
    pub(crate) fn system_exchange_GetRet_struct_get_type() -> FeatureType;
    pub(crate) fn system_exchange_RemoveInfo_struct_get_type() -> FeatureType;
    pub(crate) fn system_exchange_SetInfo_struct_get_type() -> FeatureType;
    pub(crate) fn system_exchange_GetInfo_struct_get_type() -> FeatureType;
    pub(crate) fn system_exchange_GetRet_ptr_promise_resolve(
        handle: FeatureInstanceHandle,
        id: FtPromiseId,
        val: *mut system_exchange_GetRet,
    ) -> FtBool;
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "ClearInfo")]
pub struct system_exchange_ClearInfo {
    scope: FtString,
}

impl ClearInfo {
    pub(crate) fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    #[allow(dead_code)]
    pub(crate) fn get_scope(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.scope) }
    }
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "GetRet")]
pub struct system_exchange_GetRet {
    value: FtString,
}

impl GetRet {
    pub(crate) fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    pub(crate) fn as_ptr(&self) -> *mut system_exchange_GetRet {
        self.inner.as_ptr()
    }

    pub(crate) fn set_value(&mut self, value: FeatureString) {
        if !self.value.is_null() {
            unsafe {
                FeatureFreeValue(self.value as *mut c_void);
            }
            self.value = ptr::null();
        }
        self.value = FeatureString::into_raw(value);
    }
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "RemoveInfo")]
pub struct system_exchange_RemoveInfo {
    key: FtString,
    scope: FtString,
}

impl RemoveInfo {
    pub(crate) fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    pub(crate) fn get_key(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.key) }
    }

    pub(crate) fn get_scope(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.scope) }
    }
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "SetInfo")]
pub struct system_exchange_SetInfo {
    key: FtString,
    value: FtString,
    scope: FtString,
}

impl SetInfo {
    pub(crate) fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    pub(crate) fn get_key(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.key) }
    }

    pub(crate) fn get_value(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.value) }
    }

    pub(crate) fn get_scope(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.scope) }
    }
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "GetInfo")]
pub struct system_exchange_GetInfo {
    key: FtString,
    scope: FtString,
}

impl GetInfo {
    pub(crate) fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    pub(crate) fn get_key(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.key) }
    }

    pub(crate) fn get_scope(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.scope) }
    }
}

#[derive(Clone, Default)]
pub(crate) struct GetRetPromise;

impl Promise for GetRetPromise {
    type Output = GetRet;

    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
        let ptr = value.as_ptr();
        unsafe {
            system_exchange_GetRet_ptr_promise_resolve(instance.as_handle(), id, ptr);
        }
    }
}

#[async_trait]
pub(crate) trait Exchange: FeatureInstanceTrait + Send + Sync {
    async fn set(&mut self, info: Option<SetInfo>) -> Result<FeatureString, PromiseError>;
    async fn get(&mut self, info: Option<GetInfo>) -> Result<GetRet, PromiseError>;
    async fn remove(&mut self, info: Option<RemoveInfo>) -> Result<FeatureString, PromiseError>;
    async fn clear(&mut self, info: Option<ClearInfo>) -> Result<FeatureString, PromiseError>;
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onRegister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().unwrap_or(""));
    system_exchange_on_register(&fname);
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onCreate(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    let boxed = Box::new(ExchangePrototype::new(proto.clone()));
    proto.attach(boxed);
    system_exchange_on_create(ctx, proto);
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onRequired(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureInstanceHandle,
) {
    let instance = FeatureInstance::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    let boxed = Box::new(ExchangeImpl::new(instance.clone())) as Box<dyn Exchange>;
    instance.attach(boxed);
    system_exchange_on_required(ctx, instance);
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onDetached(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureInstanceHandle,
) {
    let instance = FeatureInstance::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    system_exchange_on_detached(ctx, instance.clone());
    let _: Option<Box<dyn Exchange>> = instance.detach();
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onDestroy(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    system_exchange_on_destroy(ctx, proto.clone());
    let _: Option<Box<ExchangePrototype>> = proto.detach();
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onUnregister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().unwrap_or(""));
    system_exchange_on_unregister(&fname);
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_wrap_set(
    handle: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
    info: *mut system_exchange_SetInfo,
) {
    let system_exchange = unsafe {
        feature_glue::get_instance_data::<dyn Exchange>(handle)
            .expect("Failed to get impl for 'Exchange' trait")
    };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<FtStringPromise>::new(pid, handle) };
    let info = unsafe { SetInfo::from_raw(info) };
    runtime::spawn(async move {
        match system_exchange.set(info).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_wrap_get(
    handle: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
    info: *mut system_exchange_GetInfo,
) {
    let system_exchange = unsafe {
        feature_glue::get_instance_data::<dyn Exchange>(handle)
            .expect("Failed to get impl for 'Exchange' trait")
    };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<GetRetPromise>::new(pid, handle) };
    let info = unsafe { GetInfo::from_raw(info) };
    runtime::spawn(async move {
        match system_exchange.get(info).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_wrap_remove(
    handle: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
    info: *mut system_exchange_RemoveInfo,
) {
    let system_exchange = unsafe {
        feature_glue::get_instance_data::<dyn Exchange>(handle)
            .expect("Failed to get impl for 'Exchange' trait")
    };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<FtStringPromise>::new(pid, handle) };
    let info = unsafe { RemoveInfo::from_raw(info) };
    runtime::spawn(async move {
        match system_exchange.remove(info).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_wrap_clear(
    handle: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
    info: *mut system_exchange_ClearInfo,
) {
    let system_exchange = unsafe {
        feature_glue::get_instance_data::<dyn Exchange>(handle)
            .expect("Failed to get impl for 'Exchange' trait")
    };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<FtStringPromise>::new(pid, handle) };
    let info = unsafe { ClearInfo::from_raw(info) };
    runtime::spawn(async move {
        match system_exchange.clear(info).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}
