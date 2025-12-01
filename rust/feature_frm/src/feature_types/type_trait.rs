use feature_sys::FeatureType;

/// A trait for types that can be used in the feature system.
pub trait FeatureTypeDescription {
    fn get_type() -> FeatureType;
}

/// A trait for types that is managed by the feature system.
/// It's created by `FeatureMalloc`, freed by `FeatureFree`, cloned by `FeatureDupValue`.
/// Note: It can't be created by the user directly, and can only be used behind `FeaturePtr`.
pub trait FeatureManagedType: FeatureTypeDescription {}

/// A trait for types that represent a primitive value(int, bool...) in the feature system.
pub trait FeatureValueType: Clone + FeatureTypeDescription {}

/// A trait for types that reprsents a reference value in the feature system.
/// It usually represents a pointer to a managed type, and can be converted to and from a raw pointer.
pub trait FeatureReferenceType: Clone {
    type Target: FeatureManagedType;
    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Option<Self>;
    fn into_raw(self) -> *mut Self::Target;
}
