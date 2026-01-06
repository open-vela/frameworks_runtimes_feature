use crate::alloc::string::ToString;
use crate::exchange::*;
use alloc::format;
use alloc::{boxed::Box, string::String};
use async_trait::async_trait;
use feature_frm::*;
use feature_macros::feature_instance;
use vdk::log::{error, info};
use vdk::property::Property;

const FILE_TAG: &str = "[jidl_feature] exchange_impl";
const EXCHANGE_PERSIST: &str = "persist.";
const EXCHANGE_PERSIST_LEN: usize = EXCHANGE_PERSIST.len();
const PROP_KEY_MAX: usize = 127;
const PROP_VALUE_MAX: usize = 255;
const SYSTEM_EXCHANGE_ERROR_CODE: i32 = 202;

pub(crate) fn system_exchange_on_register(_name: &FeatureString) {}

pub(crate) fn system_exchange_on_create(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub(crate) fn system_exchange_on_required(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
}

pub(crate) fn system_exchange_on_detached(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
}

pub(crate) fn system_exchange_on_destroy(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub(crate) fn system_exchange_on_unregister(_name: &FeatureString) {}

pub(crate) struct ExchangePrototype {
    #[allow(dead_code)]
    pub(crate) proto: FeaturePrototype,
}

impl ExchangePrototype {
    pub(crate) fn new(proto: FeaturePrototype) -> Self {
        ExchangePrototype { proto }
    }
}

#[feature_instance(name = "Exchange")]
pub(crate) struct ExchangeImpl {
    pub(crate) instance: FeatureInstance,
    pub(crate) prop: Property,
}

impl ExchangeImpl {
    pub(crate) fn new(instance: FeatureInstance) -> Self {
        ExchangeImpl {
            instance,
            prop: Property,
        }
    }
}

// Enum
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum ExchangeOp {
    Get,
    Set,
    Remove,
}

pub(crate) fn process_properties(
    op: ExchangeOp,
    key: &Option<FeatureString>,
    value: &Option<FeatureString>,
    scope: &Option<FeatureString>,
) -> Result<String, PromiseError> {
    let key = key
        .as_ref()
        .filter(|k| !k.is_empty())
        .ok_or_else(|| PromiseError::new(SYSTEM_EXCHANGE_ERROR_CODE, "key is null or empty"))?;

    let scope = scope
        .as_ref()
        .filter(|s| !s.is_empty())
        .ok_or_else(|| PromiseError::new(SYSTEM_EXCHANGE_ERROR_CODE, "scope is null or empty"))?;

    if op == ExchangeOp::Set {
        let _ = value
            .as_ref()
            .filter(|v| !v.is_empty() && v.len() <= PROP_VALUE_MAX)
            .ok_or_else(|| PromiseError::new(SYSTEM_EXCHANGE_ERROR_CODE, "invalid value"))?;
    }

    if !(scope.as_str() == "vendor" || scope.as_str() == "global") {
        return Err(PromiseError::new(
            SYSTEM_EXCHANGE_ERROR_CODE,
            format!("scope {scope} not support"),
        ));
    }

    let scope_len = scope.len();
    let key_len = EXCHANGE_PERSIST_LEN + scope_len + 1 // '.'
        + key.len()
        + 1 // '\0'
        ;
    if key_len > PROP_KEY_MAX {
        return Err(PromiseError::new(
            SYSTEM_EXCHANGE_ERROR_CODE,
            format!("key length too long, max:{PROP_KEY_MAX}, current:{key_len}"),
        ));
    }

    Ok(format!("{EXCHANGE_PERSIST}{scope}.{key}"))
}

#[async_trait]
impl Exchange for ExchangeImpl {
    async fn set(&mut self, info: Option<SetInfo>) -> Result<FeatureString, PromiseError> {
        let info = if let Some(info) = info {
            info
        } else {
            error!("system_exchange set: info is none");
            return Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                "info is none",
            ));
        };
        info!("{} exchange.set called", FILE_TAG);

        let key = process_properties(
            ExchangeOp::Set,
            &info.get_key(),
            &info.get_value(),
            &info.get_scope(),
        )?;
        let value = info
            .get_value()
            .as_ref()
            .expect("Failed to get value!")
            .to_string();

        match self.prop.set_worker(&key, &value).await {
            Ok(_) => Ok(FeatureString::from(value)),
            Err(e) => Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                format!("set failed: {e}"),
            )),
        }
    }

    async fn get(&mut self, info: Option<GetInfo>) -> Result<GetRet, PromiseError> {
        let info = if let Some(info) = info {
            info
        } else {
            error!("system_exchange get: info is none");
            return Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                "info is none",
            ));
        };
        info!("{} exchange.get called", FILE_TAG);

        let key = process_properties(ExchangeOp::Get, &info.get_key(), &None, &info.get_scope())?;

        match self.prop.get_worker(&key).await {
            Ok(value) => {
                let mut get_ret = GetRet::new();
                get_ret.set_value(FeatureString::new(String::from_utf8_lossy(&value)));
                Ok(get_ret)
            }
            Err(e) => Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                format!("get failed: {e}"),
            )),
        }
    }

    async fn remove(&mut self, info: Option<RemoveInfo>) -> Result<FeatureString, PromiseError> {
        let info = if let Some(info) = info {
            info
        } else {
            error!("system_exchange remove: info is none");
            return Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                "info is none",
            ));
        };
        info!("{} exchange.remove called", FILE_TAG);

        let key = process_properties(
            ExchangeOp::Remove,
            &info.get_key(),
            &None,
            &info.get_scope(),
        )?;

        match self.prop.delete_worker(&key).await {
            Ok(_) => Ok(FeatureString::from("success")),
            Err(e) => Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                format!("remove failed: {e}"),
            )),
        }
    }

    async fn clear(&mut self, info: Option<ClearInfo>) -> Result<FeatureString, PromiseError> {
        let info = if let Some(info) = info {
            info
        } else {
            error!("system_exchange clear: info is none");
            return Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                "info is none",
            ));
        };
        info!("{} exchange.clear called", FILE_TAG);

        let scope = if let Some(scope) = info.get_scope() {
            scope
        } else {
            error!("system_exchange clear: scope is none");
            return Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                "scope is none",
            ));
        };

        if !(scope.as_str() == "vendor" || scope.as_str() == "global") {
            return Err(PromiseError::new(
                SYSTEM_EXCHANGE_ERROR_CODE,
                format!("scope {scope} not support"),
            ));
        }

        Ok(FeatureString::from("success"))
    }
}

impl Drop for ExchangeImpl {
    fn drop(&mut self) {
        info!("ExchangeImpl droped");
    }
}
