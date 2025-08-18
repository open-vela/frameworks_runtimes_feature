use crate::alloc::string::ToString;
use crate::exchange::*;
use alloc::format;
use alloc::{boxed::Box, string::String};
use async_trait::async_trait;
use feature_frm::*;
use feature_macros::feature_instance;
use vdk::property::Property;
use vdk::syslog::info;

const FILE_TAG: &str = "[jidl_feature] exchange_impl";
const EXCHANGE_PERSIST: &str = "persist.";
const EXCHANGE_PERSIST_LEN: usize = EXCHANGE_PERSIST.len();
const PROP_KEY_MAX: usize = 127;
const PROP_VALUE_MAX: usize = 255;

pub(crate) fn system_exchange_on_register(name: &FeatureString) {
    info!("ExchangeImpl on_register: {}", name.as_str())
}

pub(crate) fn system_exchange_on_create(_ctx: FeatureRuntimeContext, proto: FeaturePrototype) {
    info!("ExchangeImpl on_create");
}

pub(crate) fn system_exchange_on_required(_ctx: FeatureRuntimeContext, instance: FeatureInstance) {
    info!("ExchangeImpl on_required");
}

pub(crate) fn system_exchange_on_detached(_ctx: FeatureRuntimeContext, instance: FeatureInstance) {
    info!("ExchangeImpl on_detached");
}

pub(crate) fn system_exchange_on_destroy(_ctx: FeatureRuntimeContext, proto: FeaturePrototype) {
    info!("ExchangeImpl on_destroy");
}

pub(crate) fn system_exchange_on_unregister(name: &FeatureString) {
    info!("ExchangeImpl on_unregister: {}", name.as_str());
}

pub(crate) struct ExchangePrototype {
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
            prop: Property::default(),
        }
    }
}

// Enum
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum ExchangeOp {
    Get,
    Set,
    Remove,
    Clear,
    None,
}

pub(crate) fn process_properties(
    op: ExchangeOp,
    key: &Option<FeatureString>,
    value: &Option<FeatureString>,
    scope: &Option<FeatureString>,
) -> Result<String, PromiseError> {
    let key = key.as_ref();
    let value = value.as_ref();
    let scope = scope.as_ref();

    if key.is_none() || key.unwrap().is_empty() {
        return Err(PromiseError::new(-1, "key is null"));
    }

    if scope.is_none() || scope.unwrap().is_empty() {
        return Err(PromiseError::new(-1, "scope is null"));
    }

    if op == ExchangeOp::Set {
        if value.as_ref().is_none()
            || value.as_ref().unwrap().is_empty()
            || value.as_ref().unwrap().len() > PROP_VALUE_MAX
        {
            return Err(PromiseError::new(-1, "invalid value"));
        }
    }

    let scope = scope.unwrap();
    let key = key.unwrap();
    if !(scope.as_str() == "vendor" || scope.as_str() == "global") {
        return Err(PromiseError::new(-1, "scope {scope} not support"));
    }

    let scope_len = scope.len();
    let key_len = EXCHANGE_PERSIST_LEN + scope_len + 1 // '.'
        + key.len()
        + 1 // '\0'
        ;
    if key_len > PROP_KEY_MAX {
        return Err(PromiseError::new(
            -1,
            format!(
                "key length too long, max:{}, current:{}",
                PROP_KEY_MAX, key_len
            ),
        ));
    }

    Ok(format!("{}{}.{}", EXCHANGE_PERSIST, scope, key))
}

#[async_trait]
impl Exchange for ExchangeImpl {
    async fn set(&mut self, info: SetInfo) -> Result<FeatureString, PromiseError> {
        info!("{} exchange.set called", FILE_TAG);

        let key = process_properties(
            ExchangeOp::Set,
            &info.get_key(),
            &info.get_value(),
            &info.get_scope(),
        )?;
        let value = info.get_value().as_ref().unwrap().to_string();

        match self.prop.set(&key, &value).await {
            Ok(_) => Ok(FeatureString::from("set success")),
            Err(e) => Err(PromiseError::new(-1, format!("set failed: {}", e))),
        }
    }

    async fn get(&mut self, info: GetInfo) -> Result<GetRet, PromiseError> {
        info!("{} exchange.get called", FILE_TAG);

        let key = process_properties(ExchangeOp::Get, &info.get_key(), &None, &info.get_scope())?;

        match self.prop.get(&key).await {
            Ok(v) => {
                if let Some(value) = v {
                    let mut get_ret = GetRet::new();
                    get_ret.set_value(FeatureString::new(String::from_utf8_lossy(&value)));
                    return Ok(get_ret);
                } else {
                    return Err(PromiseError::new(-1, "property not found"));
                }
            }
            Err(e) => Err(PromiseError::new(-1, format!("get failed: {}", e))),
        }
    }

    async fn remove(&mut self, info: RemoveInfo) -> Result<FeatureString, PromiseError> {
        info!("{} exchange.remove called", FILE_TAG);

        let key = process_properties(
            ExchangeOp::Remove,
            &info.get_key(),
            &None,
            &info.get_scope(),
        )?;

        match self.prop.delete(&key).await {
            Ok(_) => Ok(FeatureString::from("remove success")),
            Err(e) => Err(PromiseError::new(-1, format!("remove failed: {}", e))),
        }
    }

    async fn clear(&mut self, info: ClearInfo) -> Result<FeatureString, PromiseError> {
        info!("{} exchange.clear called", FILE_TAG);

        Ok(FeatureString::from("clear success"))
    }
}

impl Drop for ExchangeImpl {
    fn drop(&mut self) {
        info!("ExchangeImpl droped");
    }
}
