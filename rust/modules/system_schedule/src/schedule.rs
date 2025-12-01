use alloc::boxed::Box;
use async_trait::async_trait;
use core::ffi::CStr;
use feature_frm::*;
use feature_macros::feature_struct;
use libc::c_void;
use vdk::async_runtime::runtime;

use crate::schedule_impl::{
    system_schedule_on_create, system_schedule_on_destroy, system_schedule_on_detached,
    system_schedule_on_register, system_schedule_on_required, system_schedule_on_unregister,
    ScheduleImpl, SchedulePrototype,
};

unsafe extern "C" {
    pub(crate) fn system_schedule_Job_struct_get_type() -> FeatureType;
    pub(crate) fn system_schedule_SuccessInfo_struct_get_type() -> FeatureType;
    pub(crate) fn system_schedule_SuccessInfo_ptr_promise_resolve(
        handle: FeatureInstanceHandle,
        id: FtPromiseId,
        val: *mut system_schedule_SuccessInfo,
    ) -> FtBool;
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "Job")]
pub struct system_schedule_Job {
    r#type: FtInt,
    timeout: FtInt64,
    interval: FtInt64,
    trigger_method: FtString,
    params: FtJsonObject,
}

impl Job {
    pub fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    pub fn get_type(&self) -> FtInt {
        self.r#type
    }

    pub fn set_type(&mut self, r#type: FtInt) {
        self.r#type = r#type;
    }

    pub fn set_timeout(&mut self, timeout: FtInt64) {
        self.timeout = timeout;
    }

    pub fn get_timeout(&self) -> FtInt64 {
        self.timeout
    }

    pub fn set_interval(&mut self, interval: FtInt64) {
        self.interval = interval;
    }

    pub fn get_interval(&self) -> FtInt64 {
        self.interval
    }

    pub fn get_trigger_method(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.trigger_method) }
    }

    pub fn set_trigger_method(&mut self, method: FeatureString) {
        if !self.trigger_method.is_null() {
            //  free old data
            unsafe {
                FeatureFreeValue(self.trigger_method as *mut c_void);
            }
        }
        self.trigger_method = FeatureString::into_raw(method);
    }

    pub fn get_params(&self) -> Option<FeatureJsonObject> {
        unsafe { FeatureJsonObject::from_raw(self.params) }
    }

    pub fn set_params(&mut self, params: FeatureJsonObject) {
        if !self.params.is_null() {
            //  free old data
            unsafe {
                FeatureFreeValue(self.params as *mut c_void);
            }
        }
        self.params = FeatureJsonObject::into_raw(params);
    }
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "SuccessInfo")]
pub struct system_schedule_SuccessInfo {
    id: FtInt,
}

impl SuccessInfo {
    pub fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    pub fn set_id(&mut self, id: FtInt) {
        self.id = id;
    }

    pub fn get_id(&self) -> FtInt {
        self.id
    }
}

#[derive(Clone, Default)]
pub(crate) struct SuccessInfoPromise;

impl Promise for SuccessInfoPromise {
    type Output = SuccessInfo;

    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
        unsafe {
            system_schedule_SuccessInfo_ptr_promise_resolve(
                instance.as_handle(),
                id,
                value.into_raw(),
            );
        }
    }
}

#[async_trait]
pub(crate) trait Schedule: FeatureInstanceTrait + Send + Sync {
    async fn schedule_job(&mut self, info: Option<Job>) -> Result<SuccessInfo, PromiseError>;
    async fn cancel(&mut self, id: FtInt) -> Result<(), PromiseError>;
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_onRegister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().unwrap_or(""));
    system_schedule_on_register(&fname);
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_onCreate(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    let boxed = Box::new(SchedulePrototype::new(proto.clone()));
    proto.attach(boxed);
    system_schedule_on_create(ctx, proto);
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_onRequired(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureInstanceHandle,
) {
    let instance = FeatureInstance::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    let boxed = Box::new(ScheduleImpl::new(instance.clone())) as Box<dyn Schedule>;
    instance.attach(boxed);
    system_schedule_on_required(ctx, instance);
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_onDetached(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureInstanceHandle,
) {
    let instance = FeatureInstance::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    system_schedule_on_detached(ctx, instance.clone());
    let _: Option<Box<dyn Schedule>> = instance.detach();
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_onDestroy(
    ctx: FeatureRuntimeContextHandle,
    handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    system_schedule_on_destroy(ctx, proto.clone());
    let _: Option<Box<SchedulePrototype>> = proto.detach();
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_onUnregister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().unwrap_or(""));
    system_schedule_on_unregister(&fname);
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_wrap_scheduleJob(
    handle: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
    job: *mut system_schedule_Job,
) {
    let schedule = unsafe {
        feature_glue::get_instance_data::<dyn Schedule>(handle)
            .expect("Failed to get impl for 'Schedule' trait")
    };
    let schedule = unsafe { &mut *schedule };
    let promise = unsafe { FeaturePromise::<SuccessInfoPromise>::new(pid, handle) };
    let job = unsafe { Job::from_raw(job) };
    runtime::spawn(async move {
        match schedule.schedule_job(job).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}

#[no_mangle]
pub(crate) extern "C" fn system_schedule_wrap_cancel(
    handle: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
    id: FtInt,
) {
    let schedule = unsafe {
        feature_glue::get_instance_data::<dyn Schedule>(handle)
            .expect("Failed to get impl for 'Schedule' trait")
    };
    let schedule = unsafe { &mut *schedule };
    let promise = unsafe { FeaturePromise::<FtVoidPromise>::new(pid, handle) };
    runtime::spawn(async move {
        match schedule.cancel(id).await {
            Ok(_) => promise.resolve(()),
            Err(e) => promise.reject(e),
        }
    });
}
