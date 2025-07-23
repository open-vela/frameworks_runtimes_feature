use crate::{FeatureManagedType, FeatureTypeDescription, FeatureValueType};
use feature_sys::{
    FeatureDupValue, FeatureFreeValue, FeatureMalloc, FeaturePrimitiveType, FeatureType, FtString,
};
use libc::strlen;
use std::{
    ffi::{c_void, CStr, CString},
    ops::Deref,
};

impl FeatureManagedType for FtString {}
impl FeatureTypeDescription for FtString {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_STRING as FeatureType
    }
}

// TODO: Add length info
// Could FtString be wrapped in FeaturePtr?
#[repr(transparent)]
pub struct FeatureString(FtString);

impl FeatureTypeDescription for FeatureString {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_STRING as FeatureType
    }
}

impl FeatureValueType for FeatureString {}

impl FeatureString {
    pub fn new<T: AsRef<str>>(s: T) -> Self {
        Self(Self::create_ft_string(s.as_ref()))
    }

    fn create_ft_string(s: &str) -> FtString {
        unsafe {
            let raw_ptr =
                FeatureMalloc(s.len() + 1, FeaturePrimitiveType::FT_STRING as FeatureType);

            // TODO: Return error
            assert!(!raw_ptr.is_null(), "out of memory when create string");

            std::ptr::copy_nonoverlapping(s.as_ptr(), raw_ptr as *mut u8, s.len());
            *((raw_ptr as *mut u8).add(s.len())) = 0;

            raw_ptr as FtString
        }
    }

    pub fn from_raw(ptr: FtString) -> Self {
        assert!(!ptr.is_null());
        Self(ptr)
    }

    pub fn dup_raw(ptr: FtString) -> Self {
        unsafe { FeatureDupValue(ptr as *mut c_void) };
        Self::from_raw(ptr)
    }

    pub fn into_raw(self) -> FtString {
        self.0
    }

    fn as_ptr(&self) -> FtString {
        self.0
    }

    // no allocation of memory
    pub fn as_str(&self) -> &str {
        assert!(!self.0.is_null());
        let c_str = unsafe { CStr::from_ptr(self.0) };
        c_str.to_str().expect("not utf8 encoded")
    }

    // will copy memory，use FeatureMalloc to allocate memory
    pub fn from_cstring(cstring: CString) -> Self {
        let bytes = cstring.as_bytes_with_nul();
        let ptr = unsafe {
            let raw_ptr =
                FeatureMalloc(bytes.len(), FeaturePrimitiveType::FT_STRING as FeatureType);
            assert!(!raw_ptr.is_null());
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), raw_ptr as *mut u8, bytes.len());
            raw_ptr as FtString
        };
        Self::from_raw(ptr)
    }

    // will copy memory，using Rust allocator to allocate memory
    pub fn to_cstring(&self) -> CString {
        unsafe {
            let len = libc::strlen(self.as_ptr());
            let slice = std::slice::from_raw_parts(self.as_ptr() as *const u8, len + 1);
            CString::from_vec_unchecked(slice.to_vec())
        }
    }

    // will allocate memory
    pub fn to_string(&self) -> String {
        self.as_str().to_owned()
    }

    pub fn is_null(&self) -> bool {
        self.0.is_null()
    }

    pub fn len(&self) -> usize {
        unsafe { strlen(self.as_ptr()) }
    }

    pub fn empty() -> Self {
        Self(std::ptr::null())
    }
}

impl Clone for FeatureString {
    fn clone(&self) -> Self {
        if !self.0.is_null() {
            unsafe {
                FeatureDupValue(self.0 as *mut c_void);
            }
        }
        Self(self.0)
    }
}

impl Drop for FeatureString {
    fn drop(&mut self) {
        unsafe {
            if !self.0.is_null() {
                FeatureFreeValue(self.0 as *mut c_void);
                self.0 = std::ptr::null_mut();
            }
        }
    }
}

impl std::fmt::Display for FeatureString {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

impl std::fmt::Debug for FeatureString {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "FeatureString({})", self.as_str())
    }
}

impl From<&str> for FeatureString {
    fn from(s: &str) -> Self {
        FeatureString::new(s)
    }
}

impl From<String> for FeatureString {
    fn from(s: String) -> Self {
        FeatureString::new(s)
    }
}

impl From<CString> for FeatureString {
    fn from(cstring: CString) -> Self {
        FeatureString::from_cstring(cstring)
    }
}

impl TryFrom<FeatureString> for String {
    type Error = std::str::Utf8Error;

    fn try_from(value: FeatureString) -> Result<Self, Self::Error> {
        Ok(value.to_string())
    }
}

impl Deref for FeatureString {
    type Target = str;

    fn deref(&self) -> &Self::Target {
        self.as_str()
    }
}
