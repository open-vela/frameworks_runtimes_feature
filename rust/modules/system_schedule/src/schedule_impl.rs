use crate::schedule::*;
use alloc::borrow::ToOwned;
use alloc::boxed::Box;
use async_trait::async_trait;
use feature_frm::*;
use feature_macros::feature_instance;
use vdk::async_runtime::runtime;
use vdk::log::{error, info};
use vdk::qapp_rpc;
use vdk::qapp_rpc::schedule;

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
        Self { proto }
    }
}

#[feature_instance(name = "Schedule")]
pub(crate) struct ScheduleImpl {
    pub(crate) instance: FeatureInstance,
    pub(crate) sc: schedule::ScheduleClient,
}

impl ScheduleImpl {
    pub(crate) fn new(instance: FeatureInstance) -> Self {
        let dc = runtime::block_on(async { qapp_rpc::client::Client::try_new().await })
            .expect("new ScheduleImpl failed");
        let sc = schedule::ScheduleClient::new(dc);
        Self { instance, sc }
    }
}

#[async_trait]
impl Schedule for ScheduleImpl {
    async fn schedule_job(&mut self, job: Option<Job>) -> Result<SuccessInfo, PromiseError> {
        let job = if let Some(job) = job {
            job
        } else {
            error!("system_schedule shcedule_job: job is none");
            return Err(PromiseError::new(-28, "job is none"));
        };
        let pkg_name = self
            .get_package_name()
            .ok_or(PromiseError::new(-27, "schedule_job failed!"))?;
        info!(
            "{} schedule.schedule_job, type: {}, timeout: {}, pkgname: {}, interval: {}, triggerMethod: {}, params: {}",
            FILE_TAG,
            job.get_type(),
            job.get_timeout(),
            pkg_name.clone(),
            job.get_interval(),
            job.get_trigger_method().as_ref().map(|s| s.as_str()).unwrap_or("none"),
            job.get_params().as_ref().map(|s| s.as_str()).unwrap_or("none"));
        let task = schedule::AddRequest::new(
            job.get_type().into(),
            job.get_timeout(),
            pkg_name,
            job.get_trigger_method()
                .as_ref()
                .map(|s| s.as_str())
                .unwrap_or("none")
                .to_owned(),
            job.get_interval(),
            job.get_params()
                .as_ref()
                .map(|s| s.as_str())
                .unwrap_or("none")
                .to_owned(),
        );
        match self.sc.add_task(task).await {
            Ok(id) => {
                info!("add task success, get id: {id}");
                let mut info = SuccessInfo::new();
                info.set_id(id as i32);
                Ok(info)
            }
            Err(e) => {
                error!("schedule.schedule_job failed: {e:?}");
                return Err(PromiseError::new(-27, "schedule_job failed!"));
            }
        }
    }

    async fn cancel(&mut self, id: FtInt) -> Result<(), PromiseError> {
        info!("{} schedule.cancel, id: {}", FILE_TAG, id);
        let dr = schedule::DeleteRequest::new(id as u64);
        match self.sc.delete_task(dr).await {
            Ok(_) => Ok(()),
            Err(e) => {
                error!("schedule.cancel failed: {e:?}");
                return Err(PromiseError::new(-26, "cancel failed!"));
            }
        }
    }
}
