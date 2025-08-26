use proc_macro::TokenStream;
use proc_macro2::Span;
use proc_macro_error::{abort, proc_macro_error};
use quote::quote;
use syn::{
    parse::{Parse, ParseStream},
    parse_macro_input,
    punctuated::Punctuated,
    Attribute, DeriveInput, FnArg, Ident, ItemFn, LitStr, Pat, ReturnType, Token, Type,
};

// for proc_macro_derive FeatureInstance
enum FeatureAttr {
    None,
    Name(String),
    Async(bool),
}

impl Parse for FeatureAttr {
    fn parse(input: ParseStream<'_>) -> syn::Result<Self> {
        let name: Ident = input.parse()?;
        let name_str = name.to_string();
        // println!("feature attr: {}", name_str);

        // name = value attributes
        if input.peek(Token![=]) {
            let _assign_token = input.parse::<Token![=]>()?; // skip '='
            if input.peek(LitStr) {
                let lit: LitStr = input.parse()?;
                let lit_str = lit.value();

                // println!("feature lit_str: {}", lit_str);
                match &*name_str {
                    "name" => Ok(FeatureAttr::Name(lit_str)),
                    "is_async" | "r#async" => Ok(FeatureAttr::Async(lit_str == "true")),
                    _ => abort!(name, "unknown vela feature attributes"),
                }
            } else {
                Ok(FeatureAttr::None)
            }
        } else {
            Ok(FeatureAttr::None)
        }
    }
}

fn parse_attrs(all_attrs: &[Attribute]) -> Vec<FeatureAttr> {
    all_attrs
        .iter()
        .filter(|attr| attr.path().is_ident("feature_attrs"))
        .flat_map(|attr| {
            attr.parse_args_with(Punctuated::<FeatureAttr, Token![,]>::parse_terminated)
                .unwrap_or_else(|e| abort!(e.span(), "Invalid feature_attrs: {}", e))
        })
        .collect()
}

fn get_feature_name(attrs: &[FeatureAttr]) -> Option<String> {
    attrs.iter().find_map(|a| {
        if let FeatureAttr::Name(name) = a {
            Some(name.clone())
        } else {
            None
        }
    })
}

fn parse_proc_macro_param(attr: TokenStream, key: &str) -> Option<String> {
    let attr: proc_macro2::TokenStream = attr.into();
    let mut iter = attr.into_iter();

    while let Some(token) = iter.next() {
        if let proc_macro2::TokenTree::Ident(ident) = token {
            if ident == key {
                if let Some(proc_macro2::TokenTree::Punct(punct)) = iter.next() {
                    if punct.as_char() == '=' {
                        if let Some(proc_macro2::TokenTree::Literal(lit)) = iter.next() {
                            let s = lit.to_string();
                            return Some(s.trim_matches('"').to_string());
                        }
                    }
                }
            }
        }
    }
    None
}

#[proc_macro_attribute]
pub fn feature_instance(attr: TokenStream, item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as syn::ItemStruct);
    let st_name = &input.ident;

    let ft_name = parse_proc_macro_param(attr, "name").unwrap();
    let _prototype_name = {
        let name_str = ft_name.to_string();
        let proto_name = format!("{name_str}Prototype");
        syn::Ident::new(&proto_name, st_name.span())
    };

    let expanded = quote! {
        #input

        impl #st_name {
            pub fn get_handle(&self) -> FeatureInstanceHandle {
                unsafe { self.instance.as_handle() }
            }
        }

        impl core::ops::Deref for #st_name {
            type Target = FeatureInstance;

            fn deref(&self) -> &Self::Target {
                &self.instance
            }
        }

        impl FeatureInstanceTrait for #st_name {}
    };

    TokenStream::from(expanded)
}

/// Defines a feature struct and generates a wrapper struct for safe FFI interaction.
///
/// This attribute macro generates a wrapper struct that provides a safe, idiomatic Rust interface
/// for interacting with a corresponding C structure. The wrapper handles memory safety, lifetime
/// management, and FFI conversions automatically.
///
/// # Parameters
///
/// - `wrapper_struct`: Specifies the name of the generated wrapper struct. This parameter is required.
/// - `with_instance`: Optional parameter that determines whether the wrapper includes a `FeatureInstance`.
///   - `true`: Generates a wrapper with both `inner` and `instance` fields, suitable for stateful operations.
///   - `false` or omitted: Generates a wrapper with only the inner field, suitable for simple data types.
///
/// # Usage Examples
///
/// ## Basic usage (without instance):
/// ```rust
/// #[feature_struct(wrapper_struct = "Chapter")]
/// pub struct simple_Chapter {
///     page_count: FtInt,
///     title: FtString,
/// }
/// ```
///
/// ## With instance support:
/// ```rust
/// #[feature_struct(wrapper_struct = "Book", with_instance = true)]
/// pub struct simple_Book {
///     book_name: FtString,
///     chap_1: *mut simple_Chapter,
/// }
/// ```
///
/// # Generated Code
///
/// - Implements the `FeatureManagedType` and `FeatureTypeDescription` traits for the original struct
///
/// if `with_instance = false` (default):
/// - Creates a  wrapper struct with only the `inner` field
/// - Provides `Default` constructors
/// - Implements `FeatureReferenceType` for raw pointer conversions
/// else:
/// - Creates a wrapper struct with both `inner` and `instance` fields
/// - Implements manual `Clone` to properly clone both fields
///
/// - Implements `Send`/`Sync` for thread safety
/// - Implements `Deref`/`DerefMut` for direct access to the underlying data
///
/// The macro also automatically implements `FeatureManagedType` and `FeatureTypeDescription`
/// for the original struct, providing type information for the feature system.
#[proc_macro_attribute]
pub fn feature_struct(attr: TokenStream, item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as syn::ItemStruct);
    let st_name = &input.ident;
    let get_type_fn_name = Ident::new(&format!("{}_struct_get_type", st_name), st_name.span());

    let wrapper_struct = parse_proc_macro_param(attr.clone(), "wrapper_struct").unwrap();
    if wrapper_struct.is_empty() {
        abort!(st_name.span(), "wrapper_struct is empty");
    }
    let wrapper_struct: Type = syn::parse_str(&wrapper_struct).unwrap_or_else(|_| {
        abort!(
            st_name.span(),
            format!("Invalid wrapper struct: {}", wrapper_struct)
        );
    });

    let with_instance = parse_proc_macro_param(attr.clone(), "with_instance").unwrap_or_default();
    let with_instance = match with_instance.as_str() {
        "true" => true,
        "false" | "" => false,
        _ => abort!(st_name.span(), "with_instance must be 'true' or 'false'"),
    };

    let trait_impls = quote! {
        impl FeatureManagedType for #st_name {}
        impl FeatureTypeDescription for #st_name {
            fn get_type() -> FeatureType {
                unsafe { #get_type_fn_name() }
            }
        }
    };

    let wrapper_impls = quote! {
        unsafe impl Send for #wrapper_struct {}
        unsafe impl Sync for #wrapper_struct {}

        impl core::ops::Deref for #wrapper_struct {
            type Target = #st_name;

            fn deref(&self) -> &Self::Target {
                &self.inner
            }
        }

        impl core::ops::DerefMut for #wrapper_struct {
            fn deref_mut(&mut self) -> &mut Self::Target {
                &mut self.inner
            }
        }
    };

    let expanded = if with_instance {
        quote! {
            #input
            #trait_impls

            pub struct #wrapper_struct {
                inner: FeaturePtr<#st_name>,
                instance: FeatureInstance,
            }

            #wrapper_impls

            impl Clone for #wrapper_struct {
                fn clone(&self) -> Self {
                    Self {
                        inner: self.inner.clone(),
                        instance: self.instance.clone(),
                    }
                }
            }
        }
    } else {
        quote! {
            #input
            #trait_impls

            #[repr(transparent)]
            #[derive(Clone)]
            pub struct #wrapper_struct {
                inner: FeaturePtr<#st_name>,
            }

            #wrapper_impls

            impl FeatureReferenceType for #wrapper_struct {
                type Target = #st_name;

                unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Self {
                    Self { inner: FeaturePtr::from_raw(raw_ptr) }
                }

                fn into_raw(self) -> *mut Self::Target {
                    self.inner.into_raw()
                }
            }

            impl Default for #wrapper_struct {
                fn default() -> Self {
                    Self::new()
                }
            }
        }
    };

    TokenStream::from(expanded)
}

fn get_feature_async(attrs: &[FeatureAttr]) -> bool {
    for a in attrs {
        if let FeatureAttr::Async(b) = a {
            return *b;
        }
    }
    false
}

#[proc_macro_derive(FeatureInstance, attributes(feature_attrs))]
#[proc_macro_error]
pub fn derive(input: TokenStream) -> TokenStream {
    let input: DeriveInput = syn::parse(input).unwrap();
    let ident = input.ident;

    let attrs = parse_attrs(&input.attrs);
    let feature_name = get_feature_name(&attrs).unwrap();
    let _feature_async = get_feature_async(&attrs);

    let _on_register_name = Ident::new(&format!("{feature_name}_onRegister"), ident.span());
    let _on_create_name = Ident::new(&format!("{feature_name}_onCreate"), ident.span());
    let _on_required_name = Ident::new(&format!("{feature_name}_onRequired"), ident.span());
    let _on_detached_name = Ident::new(&format!("{feature_name}_onDetached"), ident.span());
    let _on_destroy_name = Ident::new(&format!("{feature_name}_onDestroy"), ident.span());
    let _on_unregister_name = Ident::new(&format!("{feature_name}_onUnregister"), ident.span());

    quote!(
    //   #[no_mangle]
    //   pub extern "C" fn #on_register_name(feature_name: *const libc::c_char) {
    //       // 可以在这里处理feature_name
    //       let name = unsafe { std::ffi::CStr::from_ptr(feature_name) };
    //       println!("wjf on_register Called from C, name: {}", name.to_str().unwrap());
    //   }

    //   #[no_mangle]
    //   pub extern "C" fn #on_create_name(ctx: *mut libc::c_void, handle: *mut libc::c_void) {
    //       // 保存ctx和handle供后续使用
    //       println!("wjf on_create Called from C");
    //   }

    //   #[no_mangle]
    //   pub extern "C" fn #on_required_name(ctx: *mut libc::c_void, handle: *mut libc::c_void) {
    //       // 处理required事件
    //       println!("wjf on_required Called from C");
    //   }

    //   #[no_mangle]
    //   pub extern "C" fn #on_detached_name(ctx: *mut libc::c_void, handle: *mut libc::c_void) {
    //       // 处理detached事件
    //       println!("wjf on_detached Called from C");
    //   }

    //   #[no_mangle]
    //   pub extern "C" fn #on_destroy_name(ctx: *mut libc::c_void, handle: *mut libc::c_void) {
    //       // 清理资源
    //       println!("wjf on_destroy Called from C");
    //   }

    //   #[no_mangle]
    //   pub extern "C" fn #on_unregister_name(feature_name: *const libc::c_char) {
    //       // 可以在这里处理feature_name
    //       let name = unsafe { std::ffi::CStr::from_ptr(feature_name) };
    //       println!("wjf on_unregister Called from C, name: {}", name.to_str().unwrap());
    //   }
    )
    .into()
}

// for proc_macro_attribute feature_method_async
struct Args {
    export_name: LitStr,
}

impl Parse for Args {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let export_name = input
            .parse::<LitStr>()
            .unwrap_or(LitStr::new("", Span::call_site()));
        Ok(Args { export_name })
    }
}

// 提取函数参数信息
fn extract_fn_info(func: &ItemFn) -> (Vec<Type>, Vec<Ident>, Option<Type>) {
    let mut arg_types = Vec::new();
    let mut arg_names = Vec::new();
    let mut return_type = None;

    // 处理参数
    for input in &func.sig.inputs {
        if let FnArg::Typed(pat_type) = input {
            arg_types.push(*pat_type.ty.clone());
            if let Pat::Ident(pat_ident) = &*pat_type.pat {
                arg_names.push(pat_ident.ident.clone());
            }
        }
    }

    // 处理返回值类型
    if let ReturnType::Type(_, ty) = &func.sig.output {
        return_type = Some(*ty.clone());
    }

    (arg_types, arg_names, return_type)
}

#[allow(dead_code)]
fn is_c_void_ptr(ty: &Type) -> bool {
    if let Type::Ptr(ptr) = ty {
        if let Type::Path(path) = &*ptr.elem {
            if let Some(seg) = path.path.segments.last() {
                return seg.ident == "c_void"
                    && path.path.segments.len() == 2
                    && path.path.segments[0].ident == "libc";
            }
        }
    }
    false
}

#[allow(dead_code)]
fn is_append_data(ty: &Type) -> bool {
    if let Type::Path(path) = ty {
        if let Some(seg) = path.path.segments.last() {
            return seg.ident == "AppendData"
                || (path.path.segments.len() == 2
                    && path.path.segments[0].ident == "crate"
                    && path.path.segments[1].ident == "ffi");
        }
    }
    false
}

fn is_type_name(ty: &Type, name: &str) -> bool {
    if let Type::Path(type_path) = ty {
        type_path
            .path
            .segments
            .last()
            .map(|s| s.ident == name)
            .unwrap_or(false)
    } else {
        false
    }
}

fn make_return_handler(
    return_type: &Option<Type>,
) -> (proc_macro2::TokenStream, proc_macro2::TokenStream) {
    let mut ret_type = quote! {*const c_void};
    let mut ret_handler = quote! {
        println!("wjf result: {:?}", result);
        Box::into_raw(Box::new(result)) as *const c_void
    };

    match return_type {
        Some(ty) => {
            println!("wjf non-void return type");
            match ty {
                Type::Path(type_path) => {
                    if let Some(seg) = type_path.path.segments.last() {
                        let ident = &seg.ident;
                        println!("path ret type: {ident}");
                        match ident.to_string().as_str() {
                            "i32" | "i64" | "f32" | "f64" | "bool" => ret_type = quote! { #ident },
                            "c_int" | "c_float" | "c_double" => {
                                ret_type = quote! { #ident };
                                ret_handler = quote! { result as #ret_type }
                            }
                            "str" => {
                                ret_handler = quote! { /* str 处理 */ }
                            }
                            "String" => {
                                ret_handler = quote! {
                                    let c_str = std::ffi::CString::new(result).unwrap();
                                    c_str.into_raw() as *const c_void
                                }
                            }
                            _ => {
                                ret_handler = quote! { /* 其他路径类型 */ }
                            }
                        }
                    }
                }
                Type::Ptr(_) => {
                    println!("wjf ptr type");
                    ret_handler = quote! { result as *const c_void }
                }
                Type::Reference(ref_type) => {
                    println!("wjf reference type");
                    // 获取引用的目标类型（去掉 & 符号后的类型）
                    let referenced_ty = &ref_type.elem;
                    if is_type_name(referenced_ty, "str") {
                        // 检查引用的目标类型是否是 str（处理 &str 情况）
                        ret_handler = quote! {
                            let c_str = std::ffi::CStr::from_ptr(result as *const i8);
                            let boxed = Box::new(c_str.to_bytes_with_nul().to_vec());
                            Box::into_raw(boxed) as *const c_void
                        }
                    } else if is_type_name(referenced_ty, "String") {
                        // 检查引用的目标类型是否是 String（处理 &String 情况）
                        ret_handler = quote! {
                            let c_str = std::ffi::CString::new(result.as_bytes()).unwrap();
                            c_str.into_raw() as *const c_void
                        }
                    } else {
                        // 其他引用类型（如 &i32）
                        ret_handler = quote! {
                            Box::into_raw(Box::new(*result)) as *const c_void
                        }
                    }
                }
                _ => {
                    println!("wjf other type");
                }
            }
        }
        None => {
            println!("wjf void return type");
            ret_type = quote! {()};
            ret_handler = quote! {}
        }
    }
    (ret_handler, ret_type)
}

static GENERATED_FREE_FN: std::sync::Once = std::sync::Once::new();

#[proc_macro_attribute]
pub fn feature_method_async(_input: TokenStream, item: TokenStream) -> TokenStream {
    let func = parse_macro_input!(item as ItemFn);
    let args = parse_macro_input!(_input as Args);
    let (arg_types, arg_names, return_type) = extract_fn_info(&func);
    let export_name = args.export_name;
    let func_name = &func.sig.ident;
    let extern_name = Ident::new(&format!("c_{func_name}"), func_name.span());

    let extra_params = if arg_types.len() > 1 {
        let extra_arg_types = &arg_types[1..];
        let extra_arg_names = &arg_names[1..];
        quote! {
            #(, #extra_arg_names: #extra_arg_types)*
        }
    } else {
        quote! {}
    };

    let extra_args = if arg_names.len() > 1 {
        let extra_arg_names = &arg_names[1..];
        quote! {
            #(, #extra_arg_names)*
        }
    } else {
        quote! {}
    };

    let function_call = quote! {
        #func_name(handle #extra_args)
    };

    let (return_handler, return_type) = make_return_handler(&return_type);

    let mut output = quote! {
        #func

        #[export_name = #export_name]
        pub extern "C" fn #extern_name(handle: *mut libc::c_void, _adata: AppendData #extra_params) -> #return_type {
            static RT: once_cell::sync::Lazy<tokio::runtime::Runtime> = once_cell::sync::Lazy::new(|| {
            Builder::new_multi_thread()
                .enable_all()
                .build()
                .expect("Failed to create Tokio runtime")
            });

            let result = RT.block_on(async {
                #function_call.await
            });

            #return_handler
        }
    };

    GENERATED_FREE_FN.call_once(|| {
        output.extend(quote! {
            #[no_mangle]
            pub extern "C" fn free_ffi_ptr(ptr: *mut c_void) {
                if !ptr.is_null() {
                    unsafe { Box::from_raw(ptr) };
                }
            }
        });
    });

    output.into()
}
