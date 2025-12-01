use crate::system_media_audio::*;
use alloc::boxed::Box;
use async_trait::async_trait;
use core::time::Duration;
use feature_frm::*;
use feature_macros::feature_instance;
use vdk::log::{error, info};
use vdk::async_runtime::time;

// const FILE_TAG: &str = "[jidl_feature] system_media_audio_impl";

pub(crate) fn system_media_audio_on_register(_name: &FeatureString) {}

pub(crate) fn system_media_audio_on_create(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub(crate) fn system_media_audio_on_required(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
}

pub(crate) fn system_media_audio_on_detached(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {
}

pub(crate) fn system_media_audio_on_destroy(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub(crate) fn system_media_audio_on_unregister(_name: &FeatureString) {}

pub(crate) struct SystemMediaAudioPrototype {
    #[allow(dead_code)]
    pub(crate) proto: FeaturePrototype,
}

impl SystemMediaAudioPrototype {
    pub(crate) fn new(proto: FeaturePrototype) -> Self {
        SystemMediaAudioPrototype { proto }
    }
}

#[feature_instance(name = "SystemMediaAudio")]
pub(crate) struct SystemMediaAudioImpl {
    pub(crate) instance: FeatureInstance,
}

impl SystemMediaAudioImpl {
    pub(crate) fn new(instance: FeatureInstance) -> Self {
        SystemMediaAudioImpl { instance }
    }
}

#[async_trait]
impl SystemMediaAudio for SystemMediaAudioImpl {
    fn create_audio_player(&mut self, _params: Option<CreatePlayerParams>) -> Option<FeatureInstance> {
        info!("SystemMediaAudio create_audio_player");
        let instance = create_audio_player_instance(&self.instance);
        instance.attach(Box::new(AudioPlayerImpl::new(instance.clone())) as Box<dyn AudioPlayer>);
        Some(instance)
    }

    fn create_audio_track(&mut self, _params: Option<CreateTrackParams>) -> Option<FeatureInstance> {
        info!("SystemMediaAudio create_audio_track");
        let instance = create_audio_track_instance(&self.instance);
        instance.attach(Box::new(AudioTrackImpl::new(instance.clone())) as Box<dyn AudioTrack>);
        Some(instance)
    }

    fn create_audio_recorder(&mut self, _params: Option<CreateRecorderParams>) -> Option<FeatureInstance> {
        info!("SystemMediaAudio create_audio_recorder");
        let instance = create_audio_recorder_instance(&self.instance);
        instance.attach(Box::new(AudioRecorderImpl::new(instance.clone())) as Box<dyn AudioRecorder>);
        Some(instance)
    }
}

impl Drop for SystemMediaAudioImpl {
    fn drop(&mut self) {
        info!("SystemMediaAudioImpl droped");
    }
}


#[feature_instance(name = "AudioPlayer")]
pub struct AudioPlayerImpl {
    instance: FeatureInstance,
}

impl AudioPlayerImpl {
    fn new(instance: FeatureInstance) -> Self {
        Self {
            instance,
        }
    }
}

#[async_trait]
impl AudioPlayer for AudioPlayerImpl {
    fn get_src(&mut self) -> Option<FeatureString> {
        info!("AudioPlayerImpl get_src");
        Some(FeatureString::new("http://www.baidu.com"))
    }

    fn set_src(&mut self, src: Option<FeatureString>) {
        let src = if let Some(src) = src {
            src
        } else {
            error!("AudioPlayer set_src: src is none");
            return;
        };
        info!("AudioPlayerImpl set_src, src: {}", src);
    }

    fn get_duration(&mut self) -> FtDouble {
        info!("AudioPlayerImpl get_duration");
        0.0
    }

    fn get_state(&mut self) -> Option<FeatureString> {
        info!("AudioPlayerImpl get_state");
        Some(FeatureString::new("ready"))
    }

    fn set_state(&mut self, state: Option<FeatureString>) {
        let state = if let Some(state) = state {
            state
        } else {
            error!("AudioPlayer set_state: state is none");
            return;
        };
        info!("AudioPlayerImpl set_state, state: {}", state);
    }

    fn get_current_time(&mut self) -> FtInt64 {
        info!("AudioPlayerImpl get_current_time");
        0
    }

    fn set_current_time(&mut self, cur_time: FtInt64) {
        info!("AudioPlayerImpl set_current_time, cur_time: {}", cur_time);
    }

    fn get_playcount(&mut self) -> FtInt {
        info!("AudioPlayerImpl get_playcount");
        0
    }

    fn set_playcount(&mut self, playcount: FtInt) {
        info!("AudioPlayerImpl set_playcount, playcount: {}", playcount);
    }

    fn play(&mut self) {
        info!("AudioPlayerImpl play");
    }

    fn pause(&mut self) {
        info!("AudioPlayerImpl pause");
    }

    fn stop(&mut self) {
        info!("AudioPlayerImpl stop");
    }

    fn release(&mut self) {
        info!("AudioPlayerImpl release");
    }

    async fn seek(&mut self, params: Option<PlayerSeekParams>) -> Result<(), PromiseError> {
        let params = if let Some(params) = params {
            params
        } else {
            error!("AudioPlayer seek: params is none");
            return Err(PromiseError::new(400, "params is none"));
        };

        info!("AudioPlayerImpl seek");
        time::sleep(Duration::from_millis(100)).await;
        let cur_time = params.get_current_time();
        if cur_time > 0 {
            Ok(())
        } else {
            Err(PromiseError::new(400, "start rejected"))
        }
    }
}

impl Drop for AudioPlayerImpl {
    fn drop(&mut self) {
        info!("AudioPlayerImpl droped");
    }
}


#[feature_instance(name = "AudioTrack")]
pub struct AudioTrackImpl {
    instance: FeatureInstance,
}

impl AudioTrackImpl {
    fn new(instance: FeatureInstance) -> Self {
        Self {
            instance,
        }
    }
}

#[async_trait]
impl AudioTrack for AudioTrackImpl {
    fn get_state(&mut self) -> Option<FeatureString> {
        info!("AudioTrackImpl get_state");
        Some(FeatureString::new("playing"))
    }

    fn set_state(&mut self, state: Option<FeatureString>) {
        let state = if let Some(state) = state {
            state
        } else {
            error!("AudioTrack set_state: state is none");
            return;
        };
        info!("AudioTrackImpl set_state, state: {}", state);
    }

    fn play(&mut self) {
        info!("AudioTrackImpl play");
    }

    async fn write(&mut self, _params: Option<TrackWriteParams>) -> Result<FtInt64, PromiseError> {
        info!("AudioTrackImpl write");
        time::sleep(Duration::from_millis(100)).await;
        Ok(100)
    }

    fn pause(&mut self) {
        info!("AudioTrackImpl pause");
    }

    fn stop(&mut self) {
        info!("AudioTrackImpl stop");
    }

    fn release(&mut self) {
        info!("AudioTrackImpl release");
    }
}


impl Drop for AudioTrackImpl {
    fn drop(&mut self) {
        info!("AudioTrackImpl droped");
    }
}

#[feature_instance(name = "AudioRecorder")]
pub struct AudioRecorderImpl {
    instance: FeatureInstance,
}

impl AudioRecorderImpl {
    fn new(instance: FeatureInstance) -> Self {
        Self {
            instance,
        }
    }
}

#[async_trait]
impl AudioRecorder for AudioRecorderImpl {
    async fn start(&mut self, params: Option<RecorderStartParams>) -> Result<FeatureString, PromiseError> {
        let params = if let Some(params) = params {
            params
        } else {
            error!("AudioRecorder start: params is none");
            return Err(PromiseError::new(400, "params is none"));
        };
        info!("AudioRecorderImpl start");
        time::sleep(Duration::from_millis(100)).await;
        if let Some(_uri) = params.get_uri() {
            Ok(FeatureString::new("start resolved!"))
        } else {
            Err(PromiseError::new(400, "start rejected"))
        }
    }

    fn pause(&mut self) {
        info!("AudioRecorderImpl pause");
    }

    fn resume(&mut self) {
        info!("AudioRecorderImpl resume");
    }

    fn stop(&mut self) {
        info!("AudioRecorderImpl stop");
    }

    fn release(&mut self) {
        info!("AudioRecorderImpl release");
    }
}


impl Drop for AudioRecorderImpl {
    fn drop(&mut self) {
        info!("AudioRecorderImpl droped");
    }
}