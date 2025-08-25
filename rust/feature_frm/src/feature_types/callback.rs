use crate::FeatureInstance;
use alloc::boxed::Box;
use feature_sys::{FeatureInstanceHandle, FeaturePostExt, FeatureRemoveCallback, FtCallbackId};

const INVALID_CALLBACK_ID: FtCallbackId = 0;

pub struct FeatureCallback {
    id: FtCallbackId,
    instance: FeatureInstance,
}

impl FeatureCallback {
    pub fn new(id: FtCallbackId, instance: FeatureInstance) -> Self {
        Self { id, instance }
    }

    pub fn post<T: FnOnce() + 'static>(&self, func: T) {
        // Box the closure and leak it to get a raw pointer
        let boxed_func = Box::new(func);
        let data_ptr = Box::into_raw(boxed_func) as u64;
        let handle = self.handle();

        unsafe {
            FeaturePostExt(handle, Some(trampoline::<T>), data_ptr);
        }
    }

    pub fn handle(&self) -> FeatureInstanceHandle {
        unsafe { self.instance.as_handle() }
    }

    pub fn id(&self) -> FtCallbackId {
        self.id
    }

    pub fn invalid_id() -> FtCallbackId {
        INVALID_CALLBACK_ID
    }

    pub fn is_valid_id(id: FtCallbackId) -> bool {
        id > INVALID_CALLBACK_ID
    }
}

impl Drop for FeatureCallback {
    fn drop(&mut self) {
        unsafe { FeatureRemoveCallback(self.handle(), self.id) };
    }
}

extern "C" fn trampoline<T: FnOnce() + 'static>(
    _status: i32,
    data: u64,
    _handle: FeatureInstanceHandle,
) {
    // Reconstruct the box and call the closure
    let func: Box<T> = unsafe { Box::from_raw(data as *mut T) };
    func();
}
