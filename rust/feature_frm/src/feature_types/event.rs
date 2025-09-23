use crate::FeatureInstance;
use alloc::boxed::Box;
use feature_sys::{FeatureInstanceHandle, FeaturePostExt, FtEventId};

pub struct FeatureEvent {
    id: FtEventId,
    instance: FeatureInstance,
}

impl FeatureEvent {
    pub fn new(id: FtEventId, instance: FeatureInstance) -> Self {
        Self { id, instance }
    }

    /// Post a closure to the main thread.
    pub fn post<T: FnOnce() + 'static>(&self, func: T) {
        // Box the closure and leak it to get a raw pointer
        let boxed_func = Box::new(func);
        let data_ptr = Box::into_raw(boxed_func) as u64;
        let handle = self.handle();

        // this C side FeaturePostExt will post the trampoline callback
        // to a worker queue running on the main thread
        unsafe {
            FeaturePostExt(handle, Some(trampoline::<T>), data_ptr);
        }
    }

    pub fn handle(&self) -> FeatureInstanceHandle {
        unsafe { self.instance.as_handle() }
    }

    pub fn id(&self) -> FtEventId {
        self.id
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
