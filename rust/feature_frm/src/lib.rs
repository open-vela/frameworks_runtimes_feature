#![no_std]
#![feature(vec_into_raw_parts)]

extern crate alloc;

mod feature_instance;
pub use feature_instance::*;

mod feature_ptr;
pub use feature_ptr::*;

mod feature_types;
pub use feature_types::*;

pub mod feature_glue;

pub use feature_sys::AppendData;
pub use feature_sys::FeatureInstanceHandle;
pub use feature_sys::FeatureInterfaceHandle;
pub use feature_sys::FeatureManagerHandle;
pub use feature_sys::FeatureProtoHandle;
pub use feature_sys::FeatureRegistryHandle;
pub use feature_sys::FeatureRuntimeContext as FeatureRuntimeContextHandle;
pub use feature_sys::FeatureType;
pub use feature_sys::FtAny;
pub use feature_sys::FtArray;
pub use feature_sys::FtArrayBuffer;
pub use feature_sys::FtBool;
pub use feature_sys::FtCallbackId;
pub use feature_sys::FtDouble;
pub use feature_sys::FtEventId;
pub use feature_sys::FtFloat;
pub use feature_sys::FtInt;
pub use feature_sys::FtInt16;
pub use feature_sys::FtInt32;
pub use feature_sys::FtInt64;
pub use feature_sys::FtInt8;
pub use feature_sys::FtJsonObject;
pub use feature_sys::FtPromiseId;
pub use feature_sys::FtString;
pub use feature_sys::FtUint16;
pub use feature_sys::FtUint32;
pub use feature_sys::FtUint64;
pub use feature_sys::FtUint8;
pub use feature_sys::NativeFunc;

pub use feature_sys::FeatureErrorCode;
pub use feature_sys::FeatureFreeInstanceHandle;
pub use feature_sys::FeatureFreeValue;

use alloc::boxed::Box;
use vdk::async_runtime::runtime::{self, Runtime};

#[no_mangle]
pub unsafe extern "C" fn init_vdk_async_runtime(
    uvloop_ptr: *mut core::ffi::c_void,
) -> *mut core::ffi::c_void {
    if uvloop_ptr.is_null() {
        vdk::log::warn!("UV loop pointer is null, skipping VDK async runtime initialization");
        return core::ptr::null_mut();
    }

    vdk::log::debug!(
        "Initializing VDK async runtime from UV loop pointer: {:p}",
        uvloop_ptr
    );

    #[allow(clippy::missing_transmute_annotations)]
    let rt = runtime::new_from_uv_loop(unsafe { core::mem::transmute(uvloop_ptr) })
        .expect("Failed to initialize VDK async runtime");
    Box::into_raw(rt) as *mut core::ffi::c_void
}

#[no_mangle]
pub extern "C" fn close_vdk_async_runtime(runtime: *mut core::ffi::c_void) {
    vdk::log::debug!("Deinitializing VDK async runtime");
    if !runtime.is_null() {
        let _ = unsafe { Box::from_raw(runtime as *mut Runtime) };
    }
}
