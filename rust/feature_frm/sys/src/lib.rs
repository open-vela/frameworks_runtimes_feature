#![no_std]
#![allow(
    non_snake_case,
    non_camel_case_types,
    non_upper_case_globals,
    unnecessary_transmutes,
    unsafe_op_in_unsafe_fn,
    clippy::all
)]

#[cfg(not(static_binding))]
include!(concat!(env!("OUT_DIR"), "/feature_framework.rs"));

#[cfg(static_binding)]
include!("feature_framework.rs");
