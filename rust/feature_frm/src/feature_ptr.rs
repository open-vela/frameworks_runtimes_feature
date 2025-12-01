use crate::feature_types::{FeatureManagedType, FeatureTypeDescription};
use core::{
    mem,
    ops::{Deref, DerefMut},
    ptr::NonNull,
};
use feature_sys::*;
use libc::c_void;

/// A smart pointer for managing types created by `FeatureMalloc` in a safe manner.
/// It ensures that the memory is properly allocated and deallocated. It's designed
/// to be clone-friendly, allowing multiple references to the same underlying feature type.
///
/// It increments the reference count of the underlying feature type when create,
/// and decrements the reference count when dropped.
#[repr(transparent)]
pub struct FeaturePtr<T: FeatureManagedType> {
    ptr: NonNull<T>,
}

impl<T: FeatureManagedType> Default for FeaturePtr<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: FeatureManagedType> FeaturePtr<T> {
    /// Creates a new `FeaturePtr` by allocating memory for the type `T`.
    pub fn new() -> Self {
        let ft = T::get_type();
        let ptr = unsafe { FeatureMalloc(mem::size_of::<T>(), ft) };
        let ptr = NonNull::new(ptr as *mut T).expect("Failed to allocate memory for FeaturePtr");
        Self { ptr }
    }

    /// Creates a `FeaturePtr` from an existing raw pointer.
    /// It increments the reference count of the underlying feature type.
    pub unsafe fn from_raw(ptr: *mut T) -> Option<Self> {
        if ptr.is_null() {
            return None;
        }
        FeatureDupValue(ptr as *mut c_void);
        let ptr = NonNull::new(ptr).expect("already checked above");
        Some(Self { ptr })
    }

    /// Consumes the `FeaturePtr` and returns the raw pointer.
    /// The caller is responsible for managing the memory afterwards.
    pub fn into_raw(self) -> *mut T {
        let ptr = self.ptr.as_ptr();
        mem::forget(self);
        ptr
    }

    /// Returns the raw pointer without consuming the `FeaturePtr`.
    pub fn as_ptr(&self) -> *mut T {
        self.ptr.as_ptr()
    }

    /// Returns a reference count to the underlying pointer.
    pub fn strong_count(&self) -> i32 {
        unsafe { FeatureGetValueRefCount(self.ptr.as_ptr() as *mut c_void) as i32 }
    }

    pub fn ptr_eq(a: &Self, b: &Self) -> bool {
        a.ptr == b.ptr
    }
}

impl<T: FeatureManagedType> Clone for FeaturePtr<T> {
    fn clone(&self) -> Self {
        unsafe {
            FeatureDupValue(self.ptr.as_ptr() as *mut c_void);
            // safety: self.ptr is non-null, so we directly unwrap here
            Self::from_raw(self.ptr.as_ptr()).expect("get null ptr from NonNull")
        }
    }
}

impl<T: FeatureManagedType> Drop for FeaturePtr<T> {
    fn drop(&mut self) {
        unsafe {
            FeatureFreeValue(self.ptr.as_ptr() as *mut c_void);
        }
    }
}

impl<T: FeatureManagedType> Deref for FeaturePtr<T> {
    type Target = T;

    fn deref(&self) -> &T {
        unsafe { self.ptr.as_ref() }
    }
}

impl<T: FeatureTypeDescription + FeatureManagedType> DerefMut for FeaturePtr<T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { self.ptr.as_mut() }
    }
}
