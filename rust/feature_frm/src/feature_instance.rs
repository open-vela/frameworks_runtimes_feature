use feature_sys::*;
use libc::{c_void, free};
use std::ffi::{CStr, CString};
use std::ptr;
use std::str::FromStr;

// common trait for all FeatureInstances
pub trait FeatureInstanceTrait {}

pub struct FeatureManager {
    handle: FeatureManagerHandle,
}

fn get_package_name(handle: FeatureProtoHandle) -> Option<String> {
    let ret = unsafe { FeatureGetPackageName(handle) };
    if ret != std::ptr::null_mut() {
        Some(String::from(
            unsafe { CStr::from_ptr(ret) }.to_str().expect(""),
        ))
    } else {
        None
    }
}

fn get_package_version(handle: FeatureProtoHandle) -> Option<String> {
    let ret = unsafe { FeatureGetPackageVersion(handle) };
    if ret != std::ptr::null_mut() {
        Some(String::from(
            unsafe { CStr::from_ptr(ret) }.to_str().expect(""),
        ))
    } else {
        None
    }
}

pub fn get_environment_name(handle: FeatureProtoHandle) -> Option<String> {
    let ret = unsafe { FeatureGetEnvironmentName(handle) };
    if ret != std::ptr::null_mut() {
        Some(String::from(
            unsafe { CStr::from_ptr(ret) }.to_str().expect(""),
        ))
    } else {
        None
    }
}

impl FeatureManager {
    pub fn new(handle: FeatureManagerHandle) -> Self {
        Self { handle: handle }
    }

    pub fn get_loop(&self) -> Option<*mut uv_loop_t> {
        let ret = unsafe { FeatureGetUVLoop(self.handle) };
        if ret != std::ptr::null_mut() {
            Some(ret)
        } else {
            None
        }
    }
}

pub struct FeatureInstance {
    handle: FeatureInstanceHandle,
}

unsafe impl Send for FeatureInstance {}
unsafe impl Sync for FeatureInstance {}

impl FeatureInstance {
    pub fn attach<T>(handle: FeatureInstanceHandle, instance: Box<T>)
    where
        T: FeatureInstanceTrait + ?Sized,
    {
        let boxed = Box::into_raw(Box::new(Box::into_raw(instance))) as *mut c_void;
        unsafe {
            FeatureSetObjectData(handle, boxed);
        }
    }

    pub fn detach<T>(handle: FeatureInstanceHandle) -> Option<Box<T>>
    where
        T: FeatureInstanceTrait + ?Sized,
    {
        let raw_ptr = unsafe { FeatureGetObjectData(handle) };
        if raw_ptr.is_null() {
            None
        } else {
            let boxed = unsafe {
                FeatureSetObjectData(handle, ptr::null_mut());
                let boxed = Box::from_raw(*(raw_ptr as *mut *mut T));
                free(raw_ptr);
                boxed
            };
            Some(boxed)
        }
    }
}

impl Clone for FeatureInstance {
    fn clone(&self) -> Self {
        FeatureInstance::new(self.handle)
    }
}

impl FeatureInstance {
    pub fn new(handle: FeatureInstanceHandle) -> Self {
        unsafe { FeatureDupInstanceHandle(handle) };
        Self { handle }
    }

    // feature instance functions
    pub fn get_manager(&self) -> FeatureManager {
        FeatureManager::new(unsafe { FeatureGetManagerHandleFromInstance(self.handle) })
    }

    pub unsafe fn as_handle(&self) -> FeatureInstanceHandle {
        return self.handle;
    }

    pub fn get_prototype<T>(&self) -> Option<*mut T> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.handle) };
        FeaturePrototype::get::<T>(proto_handle)
    }

    pub fn is_detached(&self) -> bool {
        unsafe { FeatureInstanceIsDetached(self.handle) }
    }

    pub fn get_event_id(&self, name: &str) -> Option<FtEventId> {
        if let Ok(name) = CString::from_str(name).to_owned() {
            Some(unsafe { FeatureGetEventId(self.handle, name.into_raw()) })
        } else {
            None
        }
    }

    pub fn get_event_name(&self, id: FtEventId) -> Option<String> {
        let ret = unsafe { FeatureGetEventName(self.handle, id) };
        if ret != std::ptr::null_mut() {
            Some(String::from(
                unsafe { CStr::from_ptr(ret) }.to_str().expect(""),
            ))
        } else {
            None
        }
    }

    pub fn get_event_callback_count(&self, id: FtEventId) -> i32 {
        unsafe { FeatureGetEventCallbackCount(self.handle, id) }
    }

    pub fn promise_reject(&self, id: FtPromiseId, code: FtInt, msg: FtString) -> bool {
        unsafe { FeaturePromiseReject(self.handle, id, code, msg) }
    }

    pub fn get_package_name(&self) -> Option<String> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.handle) };
        get_package_name(proto_handle)
    }

    pub fn get_package_version(&self) -> Option<String> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.handle) };
        get_package_version(proto_handle)
    }

    pub fn get_environment_name(&self) -> Option<String> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.handle) };
        get_environment_name(proto_handle)
    }
}

impl Drop for FeatureInstance {
    fn drop(&mut self) {
        unsafe { FeatureFreeInstanceHandle(self.handle) }
    }
}

pub struct FeaturePrototype {
    handle: FeatureProtoHandle,
}

impl FeaturePrototype {
    pub fn attach<T>(handle: FeatureProtoHandle, prototype: Box<T>) {
        let boxed = Box::into_raw(Box::new(Box::into_raw(prototype))) as *mut c_void;
        unsafe {
            FeatureSetProtoData(handle, boxed);
        }
    }

    pub fn detach<T>(handle: FeatureProtoHandle) -> Option<Box<T>> {
        let raw_ptr = unsafe { FeatureGetProtoData(handle) };
        if raw_ptr.is_null() {
            None
        } else {
            let boxed = unsafe {
                // clear user data
                FeatureSetProtoData(handle, ptr::null_mut());
                let boxed = Box::from_raw(*(raw_ptr as *mut *mut T));
                free(raw_ptr);
                boxed
            };
            Some(boxed)
        }
    }

    // only for internel FeatureInstance use
    fn get<T>(handle: FeatureProtoHandle) -> Option<*mut T> {
        let raw_ptr = unsafe { FeatureGetProtoData(handle) };

        if raw_ptr.is_null() {
            None
        } else {
            let boxed = unsafe { *(raw_ptr as *mut *mut T) };
            Some(boxed)
        }
    }

    pub fn get_package_name(&self) -> Option<String> {
        get_package_name(self.handle)
    }

    pub fn get_package_version(&self) -> Option<String> {
        get_package_version(self.handle)
    }

    pub fn get_environment_name(&self) -> Option<String> {
        get_environment_name(self.handle)
    }
}

impl FeaturePrototype {
    pub fn new(handle: FeatureProtoHandle) -> Self {
        Self { handle }
    }

    pub fn get_manager(&self) -> FeatureManager {
        FeatureManager::new(unsafe { FeatureGetManagerHandleFromProto(self.handle) })
    }
}
