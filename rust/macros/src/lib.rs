use proc_macro::TokenStream;
use proc_macro_error::abort;
use quote::quote;
use syn::{parse_macro_input, Ident, Type};

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

    let ft_name = parse_proc_macro_param(attr, "name").expect("Failed to get the 'name' param!");
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
///   else:
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
    let get_type_fn_name = Ident::new(&format!("{st_name}_struct_get_type"), st_name.span());

    let wrapper_struct = parse_proc_macro_param(attr.clone(), "wrapper_struct")
        .expect("Failed to get the 'wrapper_struct' param!");
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

                unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Option<Self> {
                    if raw_ptr.is_null() {
                        return None;
                    }
                    Some(Self { inner: FeaturePtr::from_raw(raw_ptr).expect("already checked above") })
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

#[proc_macro_attribute]
pub fn feature_promise(attr: TokenStream, item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as syn::ItemStruct);
    let st_name = &input.ident;

    let c_type =
        parse_proc_macro_param(attr.clone(), "c_type").expect("Failed to get the 'c_type' param!");
    if c_type.is_empty() {
        abort!(st_name.span(), "c_type is empty");
    }
    let mut out_type = parse_proc_macro_param(attr.clone(), "out_type").unwrap_or_default();
    if out_type.is_empty() {
        out_type = c_type.clone();
    }
    let c_type = Ident::new(&c_type, st_name.span());
    let out_type = Ident::new(&out_type, st_name.span());
    let resolve_fn_name = Ident::new(&format!("Feature{}PromiseResolve", c_type), st_name.span());
    let needs_ptr = out_type != c_type;

    let resolve_body = if needs_ptr {
        quote! {
            let ptr = value.as_ptr();
            unsafe {
                #resolve_fn_name(instance.as_handle(), id, ptr);
            }
        }
    } else {
        quote! {
            unsafe {
                #resolve_fn_name(instance.as_handle(), id, value);
            }
        }
    };

    let expanded = quote! {
        #input

        impl Promise for #st_name {
            type Output = #out_type;

            fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
                #resolve_body
            }
        }
    };

    TokenStream::from(expanded)
}
