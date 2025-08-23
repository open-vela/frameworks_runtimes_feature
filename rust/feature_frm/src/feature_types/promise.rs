use crate::{FeatureInstance, FeatureString};
use core::marker::PhantomData;
use feature_sys::{FeatureInstanceHandle, FtPromiseId};

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
