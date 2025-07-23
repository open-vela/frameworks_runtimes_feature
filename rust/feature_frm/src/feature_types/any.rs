use crate::feature_types::{FeatureManagedType, FeatureTypeDescription};
use feature_sys::{FeaturePrimitiveType, FeatureType, FtAny};

impl FeatureManagedType for FtAny {}
impl FeatureTypeDescription for FtAny {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_ANY_REF as FeatureType
    }
}
