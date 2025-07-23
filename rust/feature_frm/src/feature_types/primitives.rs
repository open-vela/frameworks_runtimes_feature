use crate::feature_types::{FeatureTypeDescription, FeatureValueType};
use feature_sys::{
    FeaturePrimitiveType, FeatureType, FtBool, FtDouble, FtFloat, FtInt, FtInt16, FtInt64, FtInt8,
    FtUint16, FtUint32, FtUint64, FtUint8,
};

// primitive types
impl FeatureValueType for FtInt8 {}
impl FeatureTypeDescription for FtInt8 {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_INT8 as FeatureType
    }
}

impl FeatureValueType for FtUint8 {}
impl FeatureTypeDescription for FtUint8 {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_UINT8 as FeatureType
    }
}

impl FeatureValueType for FtInt16 {}
impl FeatureTypeDescription for FtInt16 {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_INT16 as FeatureType
    }
}

impl FeatureValueType for FtUint16 {}
impl FeatureTypeDescription for FtUint16 {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_UINT16 as FeatureType
    }
}

impl FeatureValueType for FtInt {}
impl FeatureTypeDescription for FtInt {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_INT as FeatureType
    }
}

impl FeatureValueType for FtUint32 {}
impl FeatureTypeDescription for FtUint32 {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_UINT32 as FeatureType
    }
}

impl FeatureValueType for FtInt64 {}
impl FeatureTypeDescription for FtInt64 {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_INT64 as FeatureType
    }
}

impl FeatureValueType for FtUint64 {}
impl FeatureTypeDescription for FtUint64 {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_UINT64 as FeatureType
    }
}

impl FeatureValueType for FtFloat {}
impl FeatureTypeDescription for FtFloat {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_FLOAT as FeatureType
    }
}

impl FeatureValueType for FtDouble {}
impl FeatureTypeDescription for FtDouble {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_DOUBLE as FeatureType
    }
}

impl FeatureValueType for FtBool {}
impl FeatureTypeDescription for FtBool {
    fn get_type() -> FeatureType {
        FeaturePrimitiveType::FT_BOOLEAN as FeatureType
    }
}
