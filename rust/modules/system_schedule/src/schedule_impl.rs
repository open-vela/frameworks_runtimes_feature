use crate::schedule::*;
use alloc::boxed::Box;
use async_trait::async_trait;
use feature_frm::*;
use feature_macros::feature_instance;
use vdk::log::info;

const FILE_TAG: &str = "[jidl_feature] schedule_impl";

pub(crate) fn system_schedule_on_register(_name: &FeatureString) {}

pub(crate) fn system_schedule_on_create(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub(crate) fn system_schedule_on_required(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
}

pub(crate) fn system_schedule_on_detached(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
}

pub(crate) fn system_schedule_on_destroy(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub(crate) fn system_schedule_on_unregister(_name: &FeatureString) {}

pub(crate) struct SchedulePrototype {
    #[allow(dead_code)]
    pub(crate) proto: FeaturePrototype,
}

impl SchedulePrototype {
    pub(crate) fn new(proto: FeaturePrototype) -> Self {
        SchedulePrototype { proto }
    }
}

#[feature_instance(name = "Schedule")]
pub(crate) struct ScheduleImpl {
    pub(crate) instance: FeatureInstance,
}

impl ScheduleImpl {
    pub(crate) fn new(instance: FeatureInstance) -> Self {
        ScheduleImpl { instance }
    }
}

#[async_trait]
impl Schedule for ScheduleImpl {
    async fn schedule_job(&mut self, job: Job) -> Result<SuccessInfo, PromiseError> {
        info!(
            "{} schedule.schedule_job, type: {}, timeout: {}, interval: {}, triggerMethod: {}, params: {}",
            FILE_TAG,
            job.get_type(),
            job.get_timeout(),
            job.get_interval(),
            job.get_trigger_method(),
            job.get_params().as_str()
        );
        if job.get_type() > 2 {
            Err(PromiseError::new(-27, "schedule_job failed!"))
        } else {
            let mut info = SuccessInfo::new();
            info.set_id(1);
            Ok(info)
        }
    }

    fn cancel(&mut self, id: FtInt) -> FtBool {
        info!("{} schedule.cancel, id: {}", FILE_TAG, id);
        true
    }
}

impl Drop for ScheduleImpl {
    fn drop(&mut self) {
        info!("ScheduleImpl droped");
    }
}
