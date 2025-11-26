use crate::{FeatureInstance, FeatureString};
use core::marker::PhantomData;
use feature_macros::feature_promise;
use feature_sys::{
    FeatureFtArrayPromiseResolve, FeatureFtBoolPromiseResolve, FeatureFtDoublePromiseResolve,
    FeatureFtFloatPromiseResolve, FeatureFtInt16PromiseResolve, FeatureFtInt64PromiseResolve,
    FeatureFtInt8PromiseResolve, FeatureFtIntPromiseResolve, FeatureFtStringPromiseResolve,
    FeatureFtUint16PromiseResolve, FeatureFtUint32PromiseResolve, FeatureFtUint64PromiseResolve,
    FeatureFtUint8PromiseResolve, FeatureFtVoidPromiseResolve, FeatureInstanceHandle, FtArray,
    FtBool, FtDouble, FtFloat, FtInt, FtInt16, FtInt64, FtInt8, FtPromiseId, FtUint16, FtUint32,
    FtUint64, FtUint8,
};

pub trait Promise {
    type Output;

    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output);
}

pub struct PromiseError {
    code: i32,
    message: FeatureString,
}

impl PromiseError {
    pub fn new<S: AsRef<str>>(code: i32, message: S) -> Self {
        let message = FeatureString::new(message);
        Self { code, message }
    }
}

#[must_use = "FeaturePromise should be resolved or rejected"]
#[derive(Clone)]
pub struct FeaturePromise<T: Promise> {
    id: FtPromiseId,
    instance: FeatureInstance,
    promise: T,
    _marker: PhantomData<fn() -> T::Output>,
}

impl<T: Promise + Default> FeaturePromise<T> {
    pub unsafe fn new(id: FtPromiseId, handle: FeatureInstanceHandle) -> Self {
        Self {
            id,
            instance: FeatureInstance::new(handle),
            promise: T::default(),
            _marker: PhantomData,
        }
    }

    pub fn resolve(self, data: T::Output) {
        self.promise.resolve(self.id, &self.instance, data)
    }

    pub fn reject(self, promise_error: PromiseError) {
        self.instance
            .promise_reject(self.id, promise_error.code, promise_error.message.as_ptr());
    }
}

// basic promises

// for JIDL promise 'FtVoid'
#[derive(Clone, Default)]
pub struct FtVoidPromise;
impl Promise for FtVoidPromise {
    type Output = ();
    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, _value: Self::Output) {
        unsafe {
            FeatureFtVoidPromiseResolve(instance.as_handle(), id);
        }
    }
}

// for JIDL promise 'FtArray'
#[derive(Clone, Default)]
pub struct FtArrayPromise;
impl Promise for FtArrayPromise {
    type Output = *mut FtArray;
    #[allow(clippy::not_unsafe_ptr_arg_deref)]
    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
        unsafe {
            FeatureFtArrayPromiseResolve(instance.as_handle(), id, value);
        }
    }
}

// for JIDL promise 'FtString'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtString", out_type = "FeatureString")]
pub struct FtStringPromise;

// for JIDL promise 'FtInt'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtInt")]
pub struct FtIntPromise;

// for JIDL promise 'FtUint32'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtUint32")]
pub struct FtUint32Promise;

// for JIDL promise 'FtInt8'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtInt8")]
pub struct FtInt8Promise;

// for JIDL promise 'FtUint8'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtUint8")]
pub struct FtUint8Promise;

// for JIDL promise 'FtInt16'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtInt16")]
pub struct FtInt16Promise;

// for JIDL promise 'FtUint16'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtUint16")]
pub struct FtUint16Promise;

// for JIDL promise 'FtInt64'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtInt64")]
pub struct FtInt64Promise;

// for JIDL promise 'FtUint64'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtUint64")]
pub struct FtUint64Promise;

// for JIDL promise 'FtFloat'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtFloat")]
pub struct FtFloatPromise;

// for JIDL promise 'FtDouble'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtDouble")]
pub struct FtDoublePromise;

// for JIDL promise 'FtBool'
#[derive(Clone, Default)]
#[feature_promise(c_type = "FtBool")]
pub struct FtBoolPromise;
