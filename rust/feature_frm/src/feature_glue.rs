// only for glue code, user code is forbiden
use crate::feature_instance::*;
use alloc::boxed::Box;
use feature_sys::*;

// create interface instance and bind user data
#[allow(dead_code)]
pub(crate) fn create_interface<T>(
    handle: FeatureInstanceHandle,
    vtable: *mut VTable,
    user_data: Box<T>,
) -> FeatureInterfaceHandle
where
    T: FeatureInstanceTrait + ?Sized,
{
    let handle = unsafe { FeatureCreateInterface(handle, vtable) };
    let instance = FeatureInstance::new(handle);
    instance.attach(user_data);

    handle
}

// get user data that was bound to the instance, only for glue code
pub unsafe fn get_instance_data<T>(handle: FeatureInstanceHandle) -> Option<*mut T>
where
    T: FeatureInstanceTrait + ?Sized,
{
    let raw_ptr = unsafe { FeatureGetObjectData(handle) };

    if raw_ptr.is_null() {
        None
    } else {
        let boxed = unsafe { *(raw_ptr as *mut *mut T) };
        Some(boxed)
    }
}
