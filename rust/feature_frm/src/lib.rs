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

pub use feature_sys::FeatureFtIntPromiseResolve;
pub use feature_sys::FeatureFtStringPromiseResolve;
pub use feature_sys::FeatureFtVoidPromiseResolve;

pub use feature_sys::FeatureFreeValue;
pub use feature_sys::FeatureInstanceFreeValue;
