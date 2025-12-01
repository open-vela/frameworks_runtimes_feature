use crate::{FeatureManagedType, FeaturePtr, FeatureTypeDescription, FeatureValueType};
use alloc::ffi::CString;
use alloc::string::{String, ToString};
use core::{
    ffi::{c_void, CStr},
    fmt,
    hash::{Hash, Hasher},
    ops::Deref,
    slice,
};
use feature_sys::{FeatureFreeValue, FeatureMalloc, FeaturePrimitiveType, FeatureType, FtString};
use libc::{c_char, strlen};

impl FeatureManagedType for c_char {}
impl FeatureTypeDescription for FtString {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_STRING as FeatureType
    }
}

#[derive(Clone)]
pub struct FeatureString(FeaturePtr<c_char>, usize);

impl FeatureTypeDescription for FeatureString {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_STRING as FeatureType
    }
}

impl FeatureValueType for FeatureString {}

impl FeatureString {
    pub fn new<T: AsRef<str>>(s: T) -> Self {
        let str = s.as_ref();
        let ft_str = Self::create_ft_string(str);
        let feature_string =
            Self::from_raw_with_len(ft_str, str.len()).expect("create_ft_string return null");
        unsafe {
            FeatureFreeValue(ft_str as *mut c_void);
        }
        feature_string
    }

    fn create_ft_string(s: &str) -> FtString {
        let slice = unsafe {
            let ptr = FeatureMalloc(s.len() + 1, FeaturePrimitiveType::FT_STRING as FeatureType);
            assert!(!ptr.is_null(), "out of memory when create string");
            slice::from_raw_parts_mut(ptr as *mut u8, s.len() + 1)
        };

        // TODO: check if there are nul-byte in original string
        slice[..s.len()].copy_from_slice(s.as_bytes());
        slice[s.len()] = 0;
        slice.as_mut_ptr() as FtString
    }

    /// Creates a `FeatureString` from a raw pointer.
    /// It increments the reference count of the underlying FtString.
    pub unsafe fn from_raw(ptr: FtString) -> Option<Self> {
        if ptr.is_null() {
            return None;
        }
        let length = unsafe { strlen(ptr) };
        Self::from_raw_with_len(ptr, length as usize)
    }

    /// Creates a `FeatureString` from a raw pointer.
    /// It increments the reference count of the underlying FtString.
    fn from_raw_with_len(ptr: FtString, len: usize) -> Option<Self> {
        if ptr.is_null() {
            return None;
        }
        let ptr = unsafe { FeaturePtr::from_raw(ptr as *mut _).expect("already checked above") };
        Some(Self(ptr, len))
    }

    pub fn into_raw(self) -> FtString {
        self.0.into_raw()
    }

    pub fn as_ptr(&self) -> FtString {
        self.0.as_ptr()
    }

    /// Returns a &str, will panic if the string is not utf8 encoded
    // No memory allocation
    pub fn as_str(&self) -> &str {
        let c_str = unsafe { CStr::from_ptr(self.0.as_ptr()) };
        c_str.to_str().expect("not utf8 encoded")
    }

    /// Length of the string, excluding the null terminator.
    pub fn len(&self) -> usize {
        self.1
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl fmt::Display for FeatureString {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

impl fmt::Debug for FeatureString {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
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
        let bytes = cstring.as_bytes_with_nul();

        let slice = unsafe {
            let raw_ptr =
                FeatureMalloc(bytes.len(), FeaturePrimitiveType::FT_STRING as FeatureType);
            assert!(!raw_ptr.is_null());
            slice::from_raw_parts_mut(raw_ptr as *mut u8, bytes.len())
        };
        slice.copy_from_slice(bytes);
        let feature_string =
            Self::from_raw_with_len(slice.as_mut_ptr() as FtString, bytes.len() - 1)
                .expect("slice is null");

        unsafe {
            FeatureFreeValue(slice.as_mut_ptr() as *mut c_void);
        }

        feature_string
    }
}

impl TryFrom<FeatureString> for String {
    type Error = core::str::Utf8Error;

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

impl Hash for FeatureString {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.as_str().hash(state);
    }
}

impl PartialEq for FeatureString {
    fn eq(&self, other: &Self) -> bool {
        self.len() == other.len() && self.as_str() == other.as_str()
    }
}
