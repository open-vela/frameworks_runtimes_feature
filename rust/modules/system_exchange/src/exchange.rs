use alloc::boxed::Box;
use async_trait::async_trait;
use core::ffi::CStr;
use core::ops::{Deref, DerefMut};
use core::ptr;
use feature_frm::*;
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
    pub(crate) fn system_exchange_system_exchange_GetRet_ptr_promise_resolve(
        handle: FeatureInstanceHandle,
        id: FtPromiseId,
        val: *mut system_exchange_GetRet,
    ) -> FtBool;
}

#[repr(C)]
#[derive(Clone)]
pub(crate) struct system_exchange_ClearInfo {
    scope: FtString,
}

impl FeatureManagedType for system_exchange_ClearInfo {}
impl FeatureTypeDescription for system_exchange_ClearInfo {
    fn get_type() -> FeatureType {
        unsafe { system_exchange_ClearInfo_struct_get_type() }
    }
}

#[repr(transparent)]
#[allow(non_camel_case_types)]
#[derive(Clone)]
pub(crate) struct ClearInfo(FeaturePtr<system_exchange_ClearInfo>);

unsafe impl Send for ClearInfo {}
unsafe impl Sync for ClearInfo {}

impl Deref for ClearInfo {
    type Target = system_exchange_ClearInfo;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for ClearInfo {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl FeatureReferenceType for ClearInfo {
    type Target = system_exchange_ClearInfo;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Self {
        Self(FeaturePtr::from_raw(raw_ptr))
    }

    fn into_raw(self) -> *mut Self::Target {
        self.0.into_raw()
    }
}

#[repr(C)]
#[derive(Clone)]
pub(crate) struct system_exchange_GetRet {
    value: FtString,
}

impl FeatureManagedType for system_exchange_GetRet {}
impl FeatureTypeDescription for system_exchange_GetRet {
    fn get_type() -> FeatureType {
        unsafe { system_exchange_GetRet_struct_get_type() }
    }
}

#[repr(transparent)]
#[allow(non_camel_case_types)]
#[derive(Clone)]
pub(crate) struct GetRet(FeaturePtr<system_exchange_GetRet>);

impl GetRet {
    pub(crate) fn new() -> Self {
        Self(FeaturePtr::new())
    }

    pub(crate) fn as_ptr(&self) -> *mut system_exchange_GetRet {
        self.0.as_ptr()
    }
}

impl Deref for GetRet {
    type Target = system_exchange_GetRet;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for GetRet {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl FeatureReferenceType for GetRet {
    type Target = system_exchange_GetRet;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Self {
        Self(FeaturePtr::from_raw(raw_ptr))
    }

    fn into_raw(self) -> *mut Self::Target {
        self.0.into_raw()
    }
}

impl GetRet {
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
pub(crate) struct system_exchange_RemoveInfo {
    key: FtString,
    scope: FtString,
}

impl FeatureManagedType for system_exchange_RemoveInfo {}
impl FeatureTypeDescription for system_exchange_RemoveInfo {
    fn get_type() -> FeatureType {
        unsafe { system_exchange_RemoveInfo_struct_get_type() }
    }
}

#[repr(transparent)]
#[allow(non_camel_case_types)]
#[derive(Clone)]
pub(crate) struct RemoveInfo(FeaturePtr<system_exchange_RemoveInfo>);

unsafe impl Send for RemoveInfo {}
unsafe impl Sync for RemoveInfo {}

impl Deref for RemoveInfo {
    type Target = system_exchange_RemoveInfo;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for RemoveInfo {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl FeatureReferenceType for RemoveInfo {
    type Target = system_exchange_RemoveInfo;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Self {
        Self(FeaturePtr::from_raw(raw_ptr))
    }

    fn into_raw(self) -> *mut Self::Target {
        self.0.into_raw()
    }
}

impl RemoveInfo {
    pub(crate) fn get_key(&self) -> Option<FeatureString> {
        if self.key.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.key) })
    }

    pub(crate) fn get_scope(&self) -> Option<FeatureString> {
        if self.scope.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.scope) })
    }
}

#[repr(C)]
#[derive(Clone)]
pub(crate) struct system_exchange_SetInfo {
    key: FtString,
    value: FtString,
    scope: FtString,
}

impl FeatureManagedType for system_exchange_SetInfo {}
impl FeatureTypeDescription for system_exchange_SetInfo {
    fn get_type() -> FeatureType {
        unsafe { system_exchange_SetInfo_struct_get_type() }
    }
}

#[repr(transparent)]
#[allow(non_camel_case_types)]
#[derive(Clone)]
pub(crate) struct SetInfo(FeaturePtr<system_exchange_SetInfo>);

unsafe impl Send for SetInfo {}
unsafe impl Sync for SetInfo {}

impl Deref for SetInfo {
    type Target = system_exchange_SetInfo;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for SetInfo {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl FeatureReferenceType for SetInfo {
    type Target = system_exchange_SetInfo;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Self {
        Self(FeaturePtr::from_raw(raw_ptr))
    }

    fn into_raw(self) -> *mut Self::Target {
        self.0.into_raw()
    }
}

impl SetInfo {
    pub(crate) fn get_key(&self) -> Option<FeatureString> {
        if self.key.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.key) })
    }

    pub(crate) fn get_value(&self) -> Option<FeatureString> {
        if self.value.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.value) })
    }

    pub(crate) fn get_scope(&self) -> Option<FeatureString> {
        if self.scope.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.scope) })
    }
}

#[repr(C)]
#[derive(Clone)]
pub(crate) struct system_exchange_GetInfo {
    key: FtString,
    scope: FtString,
}

impl FeatureManagedType for system_exchange_GetInfo {}
impl FeatureTypeDescription for system_exchange_GetInfo {
    fn get_type() -> FeatureType {
        unsafe { system_exchange_GetInfo_struct_get_type() }
    }
}

#[repr(transparent)]
#[allow(non_camel_case_types)]
#[derive(Clone)]
pub(crate) struct GetInfo(FeaturePtr<system_exchange_GetInfo>);

unsafe impl Send for GetInfo {}
unsafe impl Sync for GetInfo {}

impl Deref for GetInfo {
    type Target = system_exchange_GetInfo;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for GetInfo {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl FeatureReferenceType for GetInfo {
    type Target = system_exchange_GetInfo;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Self {
        Self(FeaturePtr::from_raw(raw_ptr))
    }

    fn into_raw(self) -> *mut Self::Target {
        self.0.into_raw()
    }
}

impl GetInfo {
    pub(crate) fn get_key(&self) -> Option<FeatureString> {
        if self.key.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.key) })
    }

    pub(crate) fn get_scope(&self) -> Option<FeatureString> {
        if self.scope.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.scope) })
    }
}

#[derive(Clone, Default)]
pub(crate) struct GetRetPromise;

impl Promise for GetRetPromise {
    type Output = GetRet;

    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
        let ptr = value.as_ptr();
        unsafe {
            system_exchange_system_exchange_GetRet_ptr_promise_resolve(
                instance.as_handle(),
                id,
                ptr,
            );
        }
    }
}

#[derive(Clone, Default)]
pub(crate) struct FeatureStringPromise;

impl Promise for FeatureStringPromise {
    type Output = FeatureString;

    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
        let ptr = value.as_ptr();
        unsafe {
            FeatureFtStringPromiseResolve(instance.as_handle(), id, ptr);
        }
    }
}

#[async_trait]
pub(crate) trait Exchange: FeatureInstanceTrait + Send + Sync {
    async fn set(&mut self, info: SetInfo) -> Result<FeatureString, PromiseError>;
    async fn get(&mut self, info: GetInfo) -> Result<GetRet, PromiseError>;
    async fn remove(&mut self, info: RemoveInfo) -> Result<FeatureString, PromiseError>;
    async fn clear(&mut self, info: ClearInfo) -> Result<FeatureString, PromiseError>;
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onRegister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().unwrap());
    system_exchange_on_register(&fname);
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_onCreate(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(handle);
    let manager = proto.get_manager();
    let uv_loop = manager.get_loop().expect("FeatureGetUVLoop failed");
    #[allow(clippy::missing_transmute_annotations)]
    runtime::init_from_uv_loop(unsafe { core::mem::transmute(uv_loop) });

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
    let fname = FeatureString::new(name.to_str().unwrap());
    system_exchange_on_unregister(&fname);
}

#[no_mangle]
pub(crate) extern "C" fn system_exchange_wrap_set(
    handle: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
    info: *mut system_exchange_SetInfo,
) {
    let system_exchange =
        unsafe { feature_glue::get_instance_data::<dyn Exchange>(handle).unwrap() };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<FeatureStringPromise>::new(pid, handle) };
    let info = unsafe { SetInfo(FeaturePtr::from_raw(info)) };
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
    let system_exchange =
        unsafe { feature_glue::get_instance_data::<dyn Exchange>(handle).unwrap() };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<GetRetPromise>::new(pid, handle) };
    let info = unsafe { GetInfo(FeaturePtr::from_raw(info)) };
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
    let system_exchange =
        unsafe { feature_glue::get_instance_data::<dyn Exchange>(handle).unwrap() };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<FeatureStringPromise>::new(pid, handle) };
    let info = unsafe { RemoveInfo(FeaturePtr::from_raw(info)) };
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
    let system_exchange =
        unsafe { feature_glue::get_instance_data::<dyn Exchange>(handle).unwrap() };
    let system_exchange = unsafe { &mut *system_exchange };
    let promise = unsafe { FeaturePromise::<FeatureStringPromise>::new(pid, handle) };
    let info = unsafe { ClearInfo(FeaturePtr::from_raw(info)) };
    runtime::spawn(async move {
        match system_exchange.clear(info).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}
