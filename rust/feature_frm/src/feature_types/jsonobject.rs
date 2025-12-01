use crate::{FeatureManagedType, FeaturePtr, FeatureReferenceType, FeatureTypeDescription};
use alloc::ffi::CString;
use core::ffi::{c_void, CStr};
use core::hash::{Hash, Hasher};
use feature_sys::{
    FeatureFreeValue, FeatureGetJsonString, FeatureNewJsonObject, FeaturePrimitiveType,
    FeatureType, FtJsonObject, _FtJsonObject,
};
use libc::strlen;

impl FeatureManagedType for _FtJsonObject {}
impl FeatureTypeDescription for _FtJsonObject {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_JSON_OBJ as FeatureType
    }
}

#[derive(Clone)]
pub struct FeatureJsonObject(FeaturePtr<_FtJsonObject>, usize);

impl FeatureReferenceType for FeatureJsonObject {
    type Target = _FtJsonObject;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Option<Self> {
        Self::from_raw(raw_ptr)
    }

    fn into_raw(self) -> *mut Self::Target {
        self.0.into_raw()
    }
}

impl FeatureJsonObject {
    pub fn new<T: AsRef<str>>(s: T) -> Self {
        let str = s.as_ref();
        let c_str = CString::new(str).expect("CString::new failed");
        let obj = unsafe { FeatureNewJsonObject(c_str.as_ptr()) };
        let ret =
            Self::from_raw_with_len(obj, str.len()).expect("FeatureNewJsonObject return null");
        unsafe {
            FeatureFreeValue(obj as *mut c_void);
        }
        ret
    }

    /// Creates a `FeatureJsonObject` from a raw pointer.
    /// It increments the reference count of the underlying FtJsonObject.
    pub unsafe fn from_raw(obj: FtJsonObject) -> Option<Self> {
        if obj.is_null() {
            return None;
        }
        let json_str = unsafe { FeatureGetJsonString(obj) };
        if json_str.is_null() {
            return None;
        }
        let len = unsafe { strlen(json_str) };
        Self::from_raw_with_len(obj, len)
    }

    fn from_raw_with_len(obj: FtJsonObject, len: usize) -> Option<Self> {
        if obj.is_null() {
            return None;
        }
        let ptr = unsafe { FeaturePtr::from_raw(obj as *mut _).expect("already checked above") };
        Some(Self(ptr, len))
    }

    pub fn into_raw(self) -> FtJsonObject {
        self.0.into_raw()
    }

    pub fn as_ptr(&self) -> FtJsonObject {
        self.0.as_ptr()
    }

    /// Returns a &str, will panic if the json string is not utf8 encoded
    // No memory allocation
    pub fn as_str(&self) -> &str {
        let json_str = unsafe { FeatureGetJsonString(self.as_ptr()) };
        let cstr = unsafe { CStr::from_ptr(json_str) };
        cstr.to_str().expect("not utf8 encoded")
    }

    /// Length of the json string, excluding the null terminator.
    pub fn len(&self) -> usize {
        self.1
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl PartialEq for FeatureJsonObject {
    fn eq(&self, other: &Self) -> bool {
        self.len() == other.len() && self.as_str() == other.as_str()
    }
}

impl Hash for FeatureJsonObject {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.as_str().hash(state);
    }
}
