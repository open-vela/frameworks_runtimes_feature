use crate::{
    FeatureInstance, FeatureManagedType, FeaturePtr, FeatureReferenceType, FeatureTypeDescription,
};
use alloc::alloc::dealloc;
use alloc::boxed::Box;
use alloc::vec;
use alloc::vec::Vec;
use core::alloc::Layout;
use core::ffi::c_void;
use core::{mem, slice};
use feature_sys::{
    FeatureArrayBufferGetData, FeatureFreeValue, FeatureNewArrayBufferCopyData,
    FeatureNewArrayBufferFromData, FeaturePrimitiveType, FeatureType, FtArrayBuffer,
    _FtArrayBuffer,
};
use vdk::log::error;

// Sealed trait for number types
mod private {
    pub trait Sealed {}

    impl Sealed for u8 {}
    impl Sealed for i8 {}
    impl Sealed for u16 {}
    impl Sealed for i16 {}
    impl Sealed for u32 {}
    impl Sealed for i32 {}
    impl Sealed for u64 {}
    impl Sealed for i64 {}
    impl Sealed for f32 {}
    impl Sealed for f64 {}
}

pub trait NumberType: Copy + Sized + private::Sealed {}

impl NumberType for u8 {}
impl NumberType for i8 {}
impl NumberType for u16 {}
impl NumberType for i16 {}
impl NumberType for u32 {}
impl NumberType for i32 {}
impl NumberType for u64 {}
impl NumberType for i64 {}
impl NumberType for f32 {}
impl NumberType for f64 {}

struct CleanupInfo {
    ptr: *mut u8,
    layout: Layout,
}

impl FeatureManagedType for _FtArrayBuffer {}
impl FeatureTypeDescription for _FtArrayBuffer {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_ARRAY_BUFFER as FeatureType
    }
}

#[derive(Clone)]
pub struct FeatureArrayBuffer {
    abuf_ptr: FeaturePtr<_FtArrayBuffer>,
}

impl FeatureReferenceType for FeatureArrayBuffer {
    type Target = _FtArrayBuffer;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Option<Self> {
        Self::from_raw(raw_ptr)
    }

    fn into_raw(self) -> *mut Self::Target {
        self.abuf_ptr.into_raw()
    }
}

impl FeatureArrayBuffer {
    /// Creates a `FeatureArrayBuffer` with the given length.
    /// It will allocate memory for the array buffer.
    pub fn new(instance: &FeatureInstance, len: usize) -> Self {
        let mut vec = vec![0; len];
        vec.shrink_to_fit();
        Self::from_vec(instance, vec)
    }

    // Creates a `FeatureArrayBuffer` from a Vec<T>.
    /// It will consume the memory of the vec.
    /// The vec must be a byte vector.
    pub fn from_vec<T>(instance: &FeatureInstance, vec: Vec<T>) -> Self
    where
        T: NumberType,
    {
        let (buff, len, cap) = vec.into_raw_parts();
        let layout = Layout::array::<T>(cap).expect("Failed to create layout for vec capacity!");
        let cleanup_info = Box::new(CleanupInfo {
            ptr: buff as *mut u8,
            layout,
        });

        let bytes = len * mem::size_of::<T>();
        let abuff = unsafe {
            FeatureNewArrayBufferFromData(
                instance.as_handle(),
                buff as *mut u8,
                bytes,
                Some(array_buffer_free_func),
                Box::into_raw(cleanup_info) as *mut c_void,
            )
        };

        let ret =
            unsafe { Self::from_raw(abuff) }.expect("FeatureNewArrayBufferFromData return null");
        unsafe {
            FeatureFreeValue(abuff as *mut c_void);
        }
        ret
    }

    // Creates a `FeatureArrayBuffer` from a slice of T.
    /// It will copy data from the slice to c side memory.
    pub fn from_slice_copy<T>(instance: &FeatureInstance, slice: &[T]) -> Self
    where
        T: NumberType,
    {
        let bytes = core::mem::size_of_val(slice);
        let buff = slice.as_ptr() as *mut u8;
        let abuff = unsafe { FeatureNewArrayBufferCopyData(instance.as_handle(), buff, bytes) };
        let ret =
            unsafe { Self::from_raw(abuff) }.expect("FeatureNewArrayBufferCopyData reutrn null");
        unsafe {
            FeatureFreeValue(abuff as *mut c_void);
        }
        ret
    }

    /// Creates a `FeatureArrayBuffer` from a c side FtArrayBuffer.
    /// It increments the reference count of the underlying FtArrayBuffer.
    pub unsafe fn from_raw(abuff: FtArrayBuffer) -> Option<Self> {
        if abuff.is_null() {
            return None;
        }
        let abuf_ptr =
            unsafe { FeaturePtr::from_raw(abuff as *mut _).expect("already checked above") };
        Some(Self { abuf_ptr })
    }

    pub fn into_raw(self) -> FtArrayBuffer {
        self.abuf_ptr.into_raw()
    }

    pub fn as_ptr(&self) -> FtArrayBuffer {
        self.abuf_ptr.as_ptr()
    }

    /// Get the underlying data as a typed slice
    /// This function will check alignment and size of the data.
    /// If the data is not aligned or size is not a multiple of the type size, it will return None.
    pub fn as_slice<T>(&self) -> Option<&[T]>
    where
        T: NumberType,
    {
        let (data, bytes) = self.get_data_and_len();
        Self::check_slice::<T>(data, bytes)
            .map(|len| unsafe { slice::from_raw_parts(data as *const T, len) })
    }

    /// Get the underlying data as a mutable typed slice
    /// This function will check alignment and size of the data.
    /// If the data is not aligned or size is not a multiple of the type size, it will return None.
    pub fn as_mut_slice<T>(&mut self) -> Option<&mut [T]>
    where
        T: NumberType,
    {
        let (data, bytes) = self.get_data_and_len();
        Self::check_slice::<T>(data, bytes)
            .map(|len| unsafe { slice::from_raw_parts_mut(data as *mut T, len) })
    }

    /// Common validation logic for slice operations
    fn check_slice<T>(data: *mut u8, bytes: usize) -> Option<usize>
    where
        T: NumberType,
    {
        if data.is_null() {
            error!("arraybuffer data is null");
            return None;
        }
        // Check alignment
        if !(data as usize).is_multiple_of(mem::align_of::<T>()) {
            error!(
                "arraybuffer buff not aligned, align_size: {}",
                mem::align_of::<T>()
            );
            return None;
        }
        if !bytes.is_multiple_of(mem::size_of::<T>()) {
            error!(
                "arraybuffer bytes not aligned, bytes: {}, type_size: {}",
                bytes,
                mem::size_of::<T>()
            );
            return None;
        }
        Some(bytes / mem::size_of::<T>())
    }

    pub fn len(&self) -> usize {
        let (_, len) = self.get_data_and_len();
        len
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    fn get_data_and_len(&self) -> (*mut u8, usize) {
        let mut len: usize = 0;
        let data =
            unsafe { FeatureArrayBufferGetData(self.abuf_ptr.as_ptr(), &mut len as *mut usize) };
        (data, len)
    }
}

unsafe extern "C" fn array_buffer_free_func(opaque: *mut c_void, buff: *mut c_void) {
    assert!(
        !opaque.is_null() && !buff.is_null(),
        "array_buffer_free_func() Received null pointer!"
    );
    let info = Box::from_raw(opaque as *mut CleanupInfo);
    if buff != info.ptr as *mut c_void {
        error!(
            "wrong buffer pointer! GC buffer: {:?}, target buffer: {:?}",
            buff, info.ptr
        );
        return;
    }
    dealloc(info.ptr, info.layout);
}
