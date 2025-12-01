use crate::feature_ptr::FeaturePtr;
use crate::feature_types::{
    FeatureManagedType, FeatureReferenceType, FeatureString, FeatureTypeDescription,
    FeatureValueType,
};
use core::marker::PhantomData;
use core::{mem, ptr, slice};
use feature_sys::{
    FeatureArrayResize, FeatureCreateArray, FeaturePrimitiveType, FeatureType,
    FeatureTypeGetValueSize, FtArray, FtString,
};
use libc::{c_void, memcpy};

impl FeatureManagedType for FtArray {}
impl FeatureTypeDescription for FtArray {
    fn get_type() -> FeatureType {
        // this FT_VOID FeatureType is just a place holder, because arrays with
        // different element types share the same FtArray struct type.
        FeaturePrimitiveType::FT_VOID as FeatureType
    }
}

#[repr(transparent)]
pub(crate) struct FeatureRawArray(FeaturePtr<FtArray>);

impl FeatureRawArray {
    fn new(capacity: usize, elem_type: FeatureType) -> Self {
        let array = unsafe {
            let raw_ptr = FeatureCreateArray(ptr::null_mut(), capacity, elem_type);
            assert!(!raw_ptr.is_null());
            FeaturePtr::<FtArray>::from_raw(raw_ptr).expect("already checked above")
        };
        Self(array)
    }

    unsafe fn from_raw(ptr: *mut FtArray) -> Option<Self> {
        if ptr.is_null() {
            return None;
        }
        Some(Self(
            FeaturePtr::from_raw(ptr).expect("already checked above"),
        ))
    }

    fn into_raw(self) -> *mut FtArray {
        self.0.into_raw()
    }

    fn len(&self) -> usize {
        self.0._size as usize
    }

    fn capacity(&self) -> usize {
        self.0._capacity as usize
    }

    fn buf(&self) -> *mut c_void {
        self.0._element
    }

    fn _add_item<T>(&mut self, item: T) {
        unsafe {
            let dest = ((self.buf() as *mut T).add(self.len())) as *mut c_void;
            memcpy(
                dest,
                &item as *const T as *const c_void,
                mem::size_of::<T>(),
            );
        }
        self.0._size += 1;
    }

    pub fn enlarge(&mut self, new_size: usize) -> bool {
        if new_size <= self.capacity() {
            false
        } else {
            unsafe {
                FeatureArrayResize(self.0.as_ptr(), new_size);
            }
            true
        }
    }

    pub fn append<T>(&mut self, item: T) {
        if self.len() >= self.capacity() {
            // TODO: enlarge by twice?
            self.enlarge(self.capacity() + 1);
        }
        self._add_item(item);
    }
}

impl Clone for FeatureRawArray {
    fn clone(&self) -> Self {
        Self(self.0.clone())
    }
}

#[repr(transparent)]
#[derive(Clone)]
pub struct FeaturePrimitiveArray<T: FeatureValueType> {
    inner: FeatureRawArray,
    elem_type: PhantomData<T>,
}

impl<T: FeatureValueType> FeaturePrimitiveArray<T> {
    pub fn new(capacity: usize) -> Self {
        let ft_array = FeatureRawArray::new(capacity, T::get_type());
        Self {
            inner: ft_array,
            elem_type: PhantomData,
        }
    }

    pub unsafe fn from_raw(ptr: *mut FtArray) -> Option<Self> {
        if ptr.is_null() {
            return None;
        }
        Some(Self {
            inner: FeatureRawArray::from_raw(ptr).expect("already checked above"),
            elem_type: PhantomData,
        })
    }

    pub fn into_raw(self) -> *mut FtArray {
        self.inner.into_raw()
    }

    pub fn len(&self) -> usize {
        self.inner.len()
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn capacity(&self) -> usize {
        self.inner.capacity()
    }

    pub fn get(&self, idx: usize) -> Option<&T> {
        if idx >= self.inner.len() {
            return None;
        }
        unsafe {
            let ptr = self.inner.buf() as *const T;
            Some(&*ptr.add(idx))
        }
    }

    pub fn set(&mut self, idx: usize, element: T) {
        if idx >= self.inner.len() {
            return;
        }
        unsafe {
            let ptr = self.inner.buf() as *mut T;
            ptr.add(idx).write(element);
        }
    }

    pub fn slice_range(&self, start: usize, end: usize) -> &[T] {
        assert!(start <= end, "start must be <= end");
        assert!(end <= self.len(), "end out of bounds");

        unsafe {
            let ptr = self.inner.buf() as *const T;
            slice::from_raw_parts(ptr.add(start), end - start)
        }
    }

    pub fn slice_range_mut(&mut self, start: usize, end: usize) -> &mut [T] {
        assert!(start <= end, "start must be <= end");
        assert!(end <= self.len(), "end out of bounds");

        unsafe {
            let ptr = self.inner.buf() as *mut T;
            slice::from_raw_parts_mut(ptr.add(start), end - start)
        }
    }

    pub fn slice(&self) -> &[T] {
        self.slice_range(0, self.len())
    }

    pub fn slice_mut(&mut self) -> &mut [T] {
        self.slice_range_mut(0, self.len())
    }

    pub fn enlarge(&mut self, new_size: usize) -> bool {
        self.inner.enlarge(new_size)
    }

    pub fn append(&mut self, item: T) {
        self.inner.append::<T>(item)
    }

    pub fn append_string(&mut self, item: FeatureString) {
        self.inner.append::<FtString>(FeatureString::into_raw(item));
    }
}

#[repr(transparent)]
#[derive(Clone)]
pub struct FeatureReferenceArray<T: FeatureReferenceType> {
    inner: FeatureRawArray,
    elem_type: PhantomData<T>,
}

impl<T: FeatureReferenceType> FeatureReferenceArray<T> {
    pub fn new(size: usize) -> Self {
        let ft_array = FeatureRawArray::new(size, T::Target::get_type());
        Self {
            inner: ft_array,
            elem_type: PhantomData,
        }
    }

    pub unsafe fn from_raw(ptr: *mut FtArray) -> Option<Self> {
        if ptr.is_null() {
            return None;
        }
        Some(Self {
            inner: FeatureRawArray::from_raw(ptr).expect("already checked above"),
            elem_type: PhantomData,
        })
    }

    pub fn into_raw(self) -> *mut FtArray {
        self.inner.into_raw()
    }

    pub fn len(&self) -> usize {
        self.inner.len()
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn capacity(&self) -> usize {
        self.inner.capacity()
    }

    pub fn get(&self, idx: usize) -> Option<T> {
        if idx >= self.inner.len() {
            return None;
        }

        let buf = self.inner.buf();
        let elem_ptr = unsafe {
            let elem_size = FeatureTypeGetValueSize(T::Target::get_type()) as usize;
            *((buf as *mut u8).add(idx * elem_size) as *mut *mut T::Target)
        };

        if elem_ptr.is_null() {
            return None;
        }
        Some(unsafe { T::from_raw(elem_ptr).expect("already checked above") })
    }

    pub fn enlarge(&mut self, new_size: usize) -> bool {
        self.inner.enlarge(new_size)
    }

    pub fn append(&mut self, item: T) {
        self.inner.append::<*mut T::Target>(item.into_raw())
    }
}

impl<T: FeatureReferenceType> Iterator for FeatureReferenceArray<T> {
    type Item = T;

    fn next(&mut self) -> Option<Self::Item> {
        todo!("Implement this when required")
    }
}
