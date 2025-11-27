extern crate bindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    // Used for local development
    if std::env::var("FEATURE_STATIC_BINDING").is_ok() {
        println!("cargo:rustc-cfg=static_binding");
        return;
    }
    let apps_dir = PathBuf::from(
        env::var("NUTTX_APPS_DIR").expect("Failed to get the 'NUTTX_APPS_DIR' env value!"),
    );
    // pass apps dir and vela dir from env
    let vela_root = apps_dir
        .parent()
        .expect("Failed to get the app dir parent!");
    let nuttx_inc_dirs: Vec<PathBuf> = env::var("NUTTX_INCLUDE_DIR")
        .expect("Failed to get the 'NUTTX_INCLUDE_DIR' env value!")
        .split(':')
        .map(PathBuf::from)
        .collect();

    let feature_include = vela_root.join("frameworks/runtimes/feature/include");
    let quickjs_include = vela_root.join("apps/interpreters/quickjs");

    let uv_include = vela_root.join("apps/system/libuv/libuv/include");
    let uv_dir = vela_root.join("apps/system/libuv/libuv");
    let protobuf_dir = vela_root.join("external/protobuf-c/protobuf-c/");
    let nuttx_libc_include = vela_root.join("nuttx/libs/libm/newlib/newlib/newlib/libc/include");

    let header_files = [
        feature_include.join("feature_context.h"),
        feature_include.join("feature_description.h"),
        feature_include.join("feature_exports.h"),
        feature_include.join("feature_main_exports.h"),
        feature_include.join("feature_permission.h"),
        feature_include.join("feature_types.h"),
    ];

    let bindings = header_files
        .iter()
        .fold(bindgen::Builder::default(), |builder, header_file| {
            builder.header(header_file.to_str().expect("Failed to get header file!"))
        })
        .size_t_is_usize(false)
        .blocklist_type("max_align_t")
        .blocklist_item("CONFIG_.*") // disable CONFIG_ macros
        .translate_enum_integer_types(false)
        .prepend_enum_name(false) // disable prepend enum name
        .rustified_enum(".*") // rustify all enums
        .size_t_is_usize(true) // use usize for size_t
        .layout_tests(false)
        .use_core()
        .ctypes_prefix("cty")
        .clang_args(nuttx_inc_dirs.iter().map(|d| format!("-I{}", d.display())))
        .clang_arg(format!("-I{}", feature_include.display()))
        .clang_arg(format!(
            "-I{}",
            feature_include
                .parent()
                .expect("Failed to get include dir parent!")
                .display()
        ))
        .clang_arg(format!("-I{}", quickjs_include.display()))
        .clang_arg(format!("-I{}", uv_include.display()))
        .clang_arg(format!("-I{}", uv_dir.display()))
        .clang_arg(format!("-I{}", protobuf_dir.display()))
        .clang_arg(format!("-I{}", nuttx_libc_include.display()))
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let output_path =
        PathBuf::from(env::var("OUT_DIR").expect("Failed to get 'OUT_DIR' env value!"))
            .join("feature_framework.rs");
    bindings
        .write_to_file(&output_path)
        .expect("Couldn't write bindings!");
}
