use alloc::borrow::ToOwned;
use alloc::boxed::Box;
use alloc::ffi::CString;
use alloc::string::{String, ToString};
use core::ffi::CStr;
use core::mem;
use core::ptr::{self, NonNull};
use core::str::FromStr;
use feature_sys::{
    uv_loop_t, FeatureDupInstanceHandle, FeatureFreeInstanceHandle, FeatureGetEnvironmentName,
    FeatureGetEventCallbackCount, FeatureGetEventId, FeatureGetEventName,
    FeatureGetManagerHandleFromInstance, FeatureGetManagerHandleFromProto, FeatureGetObjectData,
    FeatureGetPackageName, FeatureGetPackageVersion, FeatureGetPathFromUri, FeatureGetProtoData,
    FeatureGetProtoHandle, FeatureGetUVLoop, FeatureInstanceHandle, FeatureInstanceIsDetached,
    FeatureManagerHandle, FeaturePromiseReject, FeatureProtoHandle, FeatureSetObjectData,
    FeatureSetProtoData, FtEventId, FtInt, FtPromiseId, FtString,
};
use libc::c_void;

// common trait for all FeatureInstances
pub trait FeatureInstanceTrait {}

/// FeatureManager is the main entry point to manage feature instances and prototypes.
#[repr(transparent)]
pub struct FeatureManager(NonNull<c_void>);

fn get_package_name(handle: FeatureProtoHandle) -> Option<String> {
    let ret = unsafe { FeatureGetPackageName(handle) };
    if ret.is_null() {
        return None;
    }

    let name = unsafe { CStr::from_ptr(ret) }
        .to_str()
        .expect("not valid UTF-8")
        .to_string();

    Some(name)
}

fn get_package_version(handle: FeatureProtoHandle) -> Option<String> {
    let ret = unsafe { FeatureGetPackageVersion(handle) };
    if ret.is_null() {
        return None;
    }

    let version = unsafe { CStr::from_ptr(ret) }
        .to_str()
        .expect("not valid UTF-8")
        .to_string();

    Some(version)
}

pub(crate) fn get_environment_name(handle: FeatureProtoHandle) -> Option<String> {
    let ret = unsafe { FeatureGetEnvironmentName(handle) };
    if ret.is_null() {
        return None;
    }

    let env_name = unsafe { CStr::from_ptr(ret) }
        .to_str()
        .expect("not valid UTF-8")
        .to_string();

    Some(env_name)
}

impl FeatureManager {
    pub fn new(handle: FeatureManagerHandle) -> Self {
        Self(NonNull::new(handle).expect("FeatureManagerHandle is null"))
    }

    fn as_ptr(&self) -> *mut c_void {
        self.0.as_ptr()
    }

    pub fn get_loop(&self) -> Option<*mut uv_loop_t> {
        let ret = unsafe { FeatureGetUVLoop(self.as_ptr()) };
        if ret.is_null() {
            return None;
        }

        Some(ret)
    }
}

#[repr(transparent)]
pub struct FeatureInstance(NonNull<c_void>);

unsafe impl Send for FeatureInstance {}
unsafe impl Sync for FeatureInstance {}

impl Clone for FeatureInstance {
    fn clone(&self) -> Self {
        FeatureInstance::new(self.as_ptr())
    }
}

impl FeatureInstance {
    #[allow(clippy::not_unsafe_ptr_arg_deref)]
    pub fn new(handle: FeatureInstanceHandle) -> Self {
        let v = Self(NonNull::new(handle).expect("FeatureInstanceHandle is null"));
        unsafe { FeatureDupInstanceHandle(handle) };
        v
    }

    pub fn attach<T>(&self, instance: Box<T>)
    where
        T: FeatureInstanceTrait + ?Sized,
    {
        let ptr = self.as_ptr();
        let old_data = unsafe { FeatureGetObjectData(ptr) };
        assert!(
            old_data.is_null(),
            "FeatureInstance already has user data attached"
        );

        let boxed = Box::into_raw(Box::new(Box::into_raw(instance))) as *mut c_void;
        unsafe {
            FeatureSetObjectData(ptr, boxed);
        }
    }

    pub fn detach<T>(&self) -> Option<Box<T>>
    where
        T: FeatureInstanceTrait + ?Sized,
    {
        let ptr = self.as_ptr();
        let user_data = unsafe { FeatureGetObjectData(ptr) };
        if user_data.is_null() {
            return None;
        }

        let boxed = unsafe {
            FeatureSetObjectData(ptr, ptr::null_mut());
            let outer_box = Box::from_raw(user_data as *mut *mut T);
            let inner_ptr = *outer_box;
            Box::from_raw(inner_ptr)
        };
        Some(boxed)
    }

    fn as_ptr(&self) -> *mut c_void {
        self.0.as_ptr()
    }

    pub fn into_raw(self) -> FeatureInstanceHandle {
        let ptr = self.as_ptr();
        mem::forget(self);
        ptr
    }

    // feature instance functions
    pub fn get_manager(&self) -> FeatureManager {
        FeatureManager::new(unsafe { FeatureGetManagerHandleFromInstance(self.as_ptr()) })
    }

    pub unsafe fn as_handle(&self) -> FeatureInstanceHandle {
        self.as_ptr()
    }

    pub fn get_prototype<T>(&self) -> Option<&T> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.as_ptr()) };
        FeaturePrototype::get::<T>(proto_handle).map(|ptr| unsafe { &*ptr })
    }

    pub fn is_detached(&self) -> bool {
        unsafe { FeatureInstanceIsDetached(self.as_ptr()) }
    }

    pub fn get_event_id(&self, name: &str) -> Option<FtEventId> {
        if let Ok(name) = CString::from_str(name).to_owned() {
            Some(unsafe { FeatureGetEventId(self.as_ptr(), name.into_raw()) })
        } else {
            None
        }
    }

    pub fn get_event_name(&self, id: FtEventId) -> Option<String> {
        let ret = unsafe { FeatureGetEventName(self.as_ptr(), id) };
        if !ret.is_null() {
            Some(String::from(
                unsafe { CStr::from_ptr(ret) }.to_str().expect(""),
            ))
        } else {
            None
        }
    }

    pub fn get_event_callback_count(&self, id: FtEventId) -> i32 {
        unsafe { FeatureGetEventCallbackCount(self.as_ptr(), id) }
    }

    pub(crate) fn promise_reject(&self, id: FtPromiseId, code: FtInt, msg: FtString) -> bool {
        unsafe { FeaturePromiseReject(self.as_ptr(), id, code, msg) }
    }

    pub fn get_package_name(&self) -> Option<String> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.as_ptr()) };
        get_package_name(proto_handle)
    }

    pub fn get_package_version(&self) -> Option<String> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.as_ptr()) };
        get_package_version(proto_handle)
    }

    pub fn get_environment_name(&self) -> Option<String> {
        let proto_handle = unsafe { FeatureGetProtoHandle(self.as_ptr()) };
        get_environment_name(proto_handle)
    }

    pub fn get_path_from_uri(&self, uri: &str) -> Option<String> {
        if let Ok(uri) = CString::from_str(uri).to_owned() {
            let result_ptr = unsafe { FeatureGetPathFromUri(self.as_ptr(), uri.into_raw()) };
            if result_ptr.is_null() {
                None
            } else {
                Some(
                    unsafe { CStr::from_ptr(result_ptr) }
                        .to_str()
                        .expect("must be utf8 string")
                        .to_string(),
                )
            }
        } else {
            None
        }
    }
}

impl Drop for FeatureInstance {
    fn drop(&mut self) {
        unsafe { FeatureFreeInstanceHandle(self.as_ptr()) }
    }
}

#[derive(Clone)]
#[repr(transparent)]
pub struct FeaturePrototype(NonNull<c_void>);

impl FeaturePrototype {
    pub fn attach<T: Sized>(&self, user_data: Box<T>) {
        let ptr = self.as_ptr();
        let old_data = unsafe { FeatureGetProtoData(ptr) };
        assert!(
            old_data.is_null(),
            "FeaturePrototype already has user data attached"
        );

        let boxed = Box::into_raw(user_data) as *mut c_void;
        unsafe {
            FeatureSetProtoData(ptr, boxed);
        }
    }

    pub fn detach<T: Sized>(&self) -> Option<Box<T>> {
        let ptr = self.as_ptr();
        let user_data = unsafe { FeatureGetProtoData(ptr) };
        if user_data.is_null() {
            None
        } else {
            let boxed = unsafe {
                // clear user data
                FeatureSetProtoData(ptr, ptr::null_mut());
                Box::from_raw(user_data as *mut T)
            };
            Some(boxed)
        }
    }

    fn as_ptr(&self) -> *mut c_void {
        self.0.as_ptr()
    }

    fn get<T: Sized>(handle: FeatureInstanceHandle) -> Option<*mut T> {
        let raw_ptr = unsafe { FeatureGetProtoData(handle) };

        if raw_ptr.is_null() {
            return None;
        }

        Some(raw_ptr as *mut T)
    }

    pub fn get_package_name(&self) -> Option<String> {
        get_package_name(self.as_ptr())
    }

    pub fn get_package_version(&self) -> Option<String> {
        get_package_version(self.as_ptr())
    }

    pub fn get_environment_name(&self) -> Option<String> {
        get_environment_name(self.as_ptr())
    }
}

impl FeaturePrototype {
    pub fn new(handle: FeatureProtoHandle) -> Self {
        Self(NonNull::new(handle).expect("FeatureProtoHandle is null"))
    }

    pub fn get_manager(&self) -> FeatureManager {
        FeatureManager::new(unsafe { FeatureGetManagerHandleFromProto(self.as_ptr()) })
    }
}

#[derive(Clone)]
#[repr(transparent)]
pub struct FeatureRuntimeContext(NonNull<c_void>);

impl FeatureRuntimeContext {
    pub fn new(handle: *mut c_void) -> Self {
        Self(NonNull::new(handle).expect("FeatureRuntimeContextHandle is null"))
    }

    pub fn as_ptr(&self) -> *mut c_void {
        self.0.as_ptr()
    }
}
