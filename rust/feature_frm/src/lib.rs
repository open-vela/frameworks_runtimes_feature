#![no_std]

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

pub use feature_sys::FeatureFreeValue;
pub use feature_sys::FeatureInstanceFreeValue;

use vdk::async_runtime::runtime;

#[no_mangle]
pub extern "C" fn init_vdk_async_runtime(uvloop_ptr: *mut core::ffi::c_void) {
    if uvloop_ptr.is_null() {
        vdk::log::warn!("UV loop pointer is null, skipping VDK async runtime initialization");
        return;
    }

    vdk::log::debug!(
        "Initializing VDK async runtime from UV loop pointer: {:p}",
        uvloop_ptr
    );

    runtime::init_from_uv_loop(unsafe { core::mem::transmute(uvloop_ptr) });
}

#[no_mangle]
pub extern "C" fn close_vdk_async_runtime() {
    vdk::log::debug!("Deinitializing VDK async runtime");
    runtime::close();
}
