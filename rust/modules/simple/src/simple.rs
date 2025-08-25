use alloc::boxed::Box;
use alloc::sync::Arc;
use async_trait::async_trait;
use core::ffi::{c_int, c_void};
use core::ops::Deref;
use core::ops::DerefMut;
use core::{ffi::CStr, ptr};
use feature_frm::*;
use vdk::async_runtime::runtime;
use vdk::log::info;

use crate::simple_impl::{
    simple_on_create, simple_on_destroy, simple_on_detached, simple_on_register,
    simple_on_required, simple_on_unregister, SimpleImpl, SimplePrototype,
};

unsafe extern "C" {
    pub fn simple_Chapter_struct_get_type() -> FeatureType;
    pub fn simple_Book_struct_get_type() -> FeatureType;
    pub fn simple_moo_cb_invoke(
        handle: FeatureInstanceHandle,
        cb: FtCallbackId,
        x: FtInt,
        y: FtString,
        z: FtDouble,
    ) -> FtInt;
    pub fn simple_chapter_changed_invoke(
        handle: FeatureInstanceHandle,
        cb: FtCallbackId,
        x: FtInt,
        y: FtString,
    ) -> FtInt;
    // Interface constructors
    pub fn simple_createDog_instance(handle: FeatureInstanceHandle) -> FeatureInterfaceHandle;
    pub fn simple_createAirplane_instance(handle: FeatureInstanceHandle) -> FeatureInterfaceHandle;
    pub fn simple_createPigeon_instance(handle: FeatureInstanceHandle) -> FeatureInterfaceHandle;
}

#[allow(non_snake_case)]
pub fn createDog_instance(instance: &FeatureInstance) -> FeatureInterfaceHandle {
    unsafe { simple_createDog_instance(instance.as_handle()) }
}

#[allow(non_snake_case)]
pub fn createAirplane_instance(instance: &FeatureInstance) -> FeatureInterfaceHandle {
    unsafe { simple_createAirplane_instance(instance.as_handle()) }
}

#[allow(non_snake_case)]
pub fn createPigeon_instance(instance: &FeatureInstance) -> FeatureInterfaceHandle {
    unsafe { simple_createPigeon_instance(instance.as_handle()) }
}

#[repr(C)]
#[derive(Clone)]
pub struct simple_Chapter_for_c {
    page_count: FtInt,
    title: FtString,
    is_end: FtBool,
}

impl FeatureManagedType for simple_Chapter_for_c {}
impl FeatureTypeDescription for simple_Chapter_for_c {
    fn get_type() -> FeatureType {
        unsafe { simple_Chapter_struct_get_type() }
    }
}

#[repr(transparent)]
#[allow(non_camel_case_types)]
#[derive(Clone)]
pub struct simple_Chapter(FeaturePtr<simple_Chapter_for_c>);

unsafe impl Send for simple_Chapter {}
unsafe impl Sync for simple_Chapter {}

impl Default for simple_Chapter {
    fn default() -> Self {
        Self::new()
    }
}

impl simple_Chapter {
    pub fn new() -> Self {
        Self(FeaturePtr::new())
    }
}

impl Deref for simple_Chapter {
    type Target = simple_Chapter_for_c;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for simple_Chapter {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl FeatureReferenceType for simple_Chapter {
    type Target = simple_Chapter_for_c;

    unsafe fn from_raw(raw_ptr: *mut Self::Target) -> Self {
        Self(FeaturePtr::from_raw(raw_ptr))
    }

    fn into_raw(self) -> *mut Self::Target {
        self.0.into_raw()
    }
}

impl simple_Chapter {
    pub fn get_page_count(&self) -> c_int {
        self.page_count
    }

    pub fn set_page_count(&mut self, count: c_int) {
        self.page_count = count;
    }

    pub fn get_title(&self) -> Option<FeatureString> {
        if self.title.is_null() {
            return None;
        }
        Some(unsafe { FeatureString::from_raw(self.title) })
    }

    pub fn set_title(&mut self, title: FeatureString) {
        if !self.title.is_null() {
            // free old title
            unsafe {
                FeatureFreeValue(self.title as *mut c_void);
            }
        }
        self.title = FeatureString::into_raw(title);
    }

    pub fn get_is_end(&self) -> FtBool {
        self.is_end
    }

    pub fn set_is_end(&mut self, is_end: FtBool) {
        self.is_end = is_end;
    }
}

pub struct ChapterChangedCb {
    cb: Arc<FeatureCallback>,
}

impl ChapterChangedCb {
    pub(crate) fn new(id: FtCallbackId, instance: FeatureInstance) -> Self {
        Self {
            cb: Arc::new(FeatureCallback::new(id, instance)),
        }
    }

    pub fn invoke(&self, index: FtInt, title: FeatureString) {
        unsafe {
            // must clone the Arc<FeatureCallback> and move it to the closure
            // to prevent the FeatureCallback from being dropped before the closure is called.
            let cb = self.cb.clone();
            self.cb.post(move || {
                let _ = simple_chapter_changed_invoke(cb.handle(), cb.id(), index, title.as_ptr());
            });
        }
    }
}

#[repr(C)]
#[derive(Clone)]
pub struct simple_Book_for_c {
    book_name: FtString,
    chap_1: *mut simple_Chapter_for_c,
    chap_changed: FtCallbackId,
}

#[allow(non_camel_case_types)]
pub struct simple_Book {
    book: FeaturePtr<simple_Book_for_c>,
    instance: FeatureInstance,
}

unsafe impl Send for simple_Book {}
unsafe impl Sync for simple_Book {}

impl Deref for simple_Book {
    type Target = simple_Book_for_c;

    fn deref(&self) -> &Self::Target {
        &self.book
    }
}

impl DerefMut for simple_Book {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.book
    }
}

impl Clone for simple_Book {
    fn clone(&self) -> Self {
        Self {
            book: self.book.clone(),
            instance: self.instance.clone(),
        }
    }
}

impl FeatureManagedType for simple_Book_for_c {}

impl FeatureTypeDescription for simple_Book_for_c {
    fn get_type() -> FeatureType {
        unsafe { simple_Book_struct_get_type() }
    }
}

impl simple_Book {
    pub fn new(book: FeaturePtr<simple_Book_for_c>, instance: FeatureInstance) -> Self {
        Self { book, instance }
    }

    pub fn get_book_name(&self) -> FeatureString {
        unsafe { FeatureString::from_raw(self.book_name) }
    }

    pub fn set_book_name(&mut self, name: FeatureString) {
        if !self.book_name.is_null() {
            //  free old data
            unsafe {
                FeatureFreeValue(self.book_name as *mut c_void);
            }
        }
        self.book_name = FeatureString::into_raw(name);
    }

    pub fn get_chap_1(&self) -> simple_Chapter {
        simple_Chapter(unsafe { FeaturePtr::from_raw(self.chap_1) })
    }

    pub fn set_chap_1(&mut self, chap_1: simple_Chapter) {
        if !self.chap_1.is_null() {
            //  free old data
            unsafe {
                FeatureFreeValue(self.chap_1 as *mut c_void);
            }
        }
        self.chap_1 = chap_1.0.into_raw()
    }

    // once the callback is taken out, the callback id will be invalid.
    pub fn take_chap_changed(&mut self) -> Option<ChapterChangedCb> {
        if FeatureCallback::is_valid_id(self.chap_changed) {
            let ret = ChapterChangedCb::new(self.chap_changed, self.instance.clone());
            self.chap_changed = FeatureCallback::invalid_id();
            Some(ret)
        } else {
            None
        }
    }
}

impl Drop for simple_Book {
    // remove the callback on drop time.
    fn drop(&mut self) {
        let _ = self.take_chap_changed();
    }
}

pub struct MooCb {
    cb: Arc<FeatureCallback>,
}

impl MooCb {
    pub(crate) fn new(id: FtCallbackId, instance: FeatureInstance) -> Self {
        Self {
            cb: Arc::new(FeatureCallback::new(id, instance)),
        }
    }

    pub fn invoke(&self, a: FtInt, b: FeatureString, c: FtDouble) {
        unsafe {
            // must clone the Arc<FeatureCallback> and move it to the closure
            // to prevent the FeatureCallback from being dropped before the closure is called.
            let cb = self.cb.clone();
            self.cb.post(move || {
                let _ = simple_moo_cb_invoke(cb.handle(), cb.id(), a, b.as_ptr(), c);
            });
        }
    }
}

#[derive(Default)]
pub struct FtIntPromise;

impl Promise for FtIntPromise {
    type Output = FtInt; // 指定关联类型

    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
        info!("Resolved with: {}", value);
        unsafe {
            FeatureFtIntPromiseResolve(instance.as_handle(), id, value);
        }
    }
}

#[derive(Default)]
pub struct FeatureStringPromise;

impl Promise for FeatureStringPromise {
    type Output = FeatureString;

    fn resolve(&self, id: FtPromiseId, instance: &FeatureInstance, value: Self::Output) {
        info!("Resolved with: {}", value.as_str());
        let ptr = value.as_ptr();
        unsafe {
            FeatureFtStringPromiseResolve(instance.as_handle(), id, ptr);
        }
    }
}

// Simple trait for FeatureInstance
#[async_trait]
pub trait Simple: FeatureInstanceTrait + Send + Sync {
    fn foo(&mut self) -> FeatureString;
    fn bar(&mut self, a: FtInt, b: FtFloat) -> FtInt;
    fn goo(&mut self, a: FtDouble) -> FtDouble;
    fn doo(&mut self);
    fn hoo(&mut self, a: &FeatureString);
    fn set_book(&mut self, book: simple_Book);
    fn get_book(&mut self) -> Option<simple_Book>;
    fn set_chapter(&mut self, chap: simple_Chapter);
    fn get_chapter(&self) -> Option<simple_Chapter>;
    fn set_chapter_array(&mut self, chap_array: FeatureReferenceArray<simple_Chapter>);
    fn get_chapter_array(&mut self) -> Option<FeatureReferenceArray<simple_Chapter>>;
    fn moo(&mut self, a: i32, cb: MooCb);
    async fn noo(&mut self, resolve: FtBool) -> Result<FtInt, PromiseError>;
    async fn poo(&mut self, resolve: FtBool) -> Result<FeatureString, PromiseError>;
    fn create_dog(&self) -> FeatureInterfaceHandle;
    fn create_airplane(&self) -> FeatureInterfaceHandle;
    fn create_pigeon(&self) -> FeatureInterfaceHandle;
    fn create_cat(&self) -> FeatureInterfaceHandle;
    fn set_animal(&self, animal: FeatureInterfaceHandle);
}

// Interface trait
pub trait Animal: FeatureInstanceTrait {
    #[allow(non_snake_case)]
    fn get_name(&self) -> FeatureString;
    #[allow(non_snake_case)]
    fn set_name(&self, name: &FeatureString);
    #[allow(non_snake_case)]
    fn get_legCount(&self) -> FtInt;
    #[allow(non_snake_case)]
    fn eatFood(&self, foods: &FeaturePrimitiveArray<FeatureString>) -> FtInt;
    #[allow(non_snake_case)]
    fn run(&self, distance: FtInt, destination: &FeatureString) -> FeatureString;
}

pub trait Flyable: FeatureInstanceTrait {
    #[allow(non_snake_case)]
    fn fly(&self) -> FeaturePrimitiveArray<FeatureString>;
    #[allow(non_snake_case)]
    fn get_breed(&self) -> FeatureString;
    #[allow(non_snake_case)]
    fn set_breed(&self, breed: &FeatureString);
}

pub trait Bird: Animal + Flyable + FeatureInstanceTrait {
    #[allow(non_snake_case)]
    fn get_weight(&self) -> FtInt;
    #[allow(non_snake_case)]
    fn set_weight(&self, weight: FtInt);
    #[allow(non_snake_case)]
    fn walk(&self, pid: FtPromiseId);
}

#[no_mangle]
pub unsafe extern "C" fn simple_onRegister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().unwrap());
    simple_on_register(&fname);
}

#[no_mangle]
pub extern "C" fn simple_onCreate(
    ctx: FeatureRuntimeContextHandle,
    proto_handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(proto_handle);
    let manager = proto.get_manager();
    let uv_loop = manager.get_loop().expect("FeatureGetUVLoop failed");
    // libuv definition is different between vdk_rs and rust framework, so we need to use unsafe to transmute it.
    // TODO: make them compatible.
    #[allow(clippy::missing_transmute_annotations)]
    runtime::init_from_uv_loop(unsafe { core::mem::transmute(uv_loop) });

    let ctx = FeatureRuntimeContext::new(ctx);
    let boxed = Box::new(SimplePrototype::new(proto.clone()));
    proto.attach(boxed);
    simple_on_create(ctx, proto);
}

#[no_mangle]
pub unsafe extern "C" fn simple_onRequired(
    ctx: FeatureRuntimeContextHandle,
    instance_handle: FeatureInstanceHandle,
) {
    let instance = FeatureInstance::new(instance_handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    let boxed = Box::new(SimpleImpl::new(instance.clone())) as Box<dyn Simple>;
    instance.attach(boxed);
    simple_on_required(ctx, instance);
}

#[no_mangle]
pub unsafe extern "C" fn simple_onDetached(
    ctx: FeatureRuntimeContextHandle,
    instance_handle: FeatureInstanceHandle,
) {
    let instance = FeatureInstance::new(instance_handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    simple_on_detached(ctx, instance.clone());
    let _: Option<Box<dyn Simple>> = instance.detach();
}

#[no_mangle]
pub extern "C" fn simple_onDestroy(
    ctx: FeatureRuntimeContextHandle,
    proto_handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(proto_handle);
    let ctx = FeatureRuntimeContext::new(ctx);
    simple_on_destroy(ctx, proto.clone());
    let _: Option<Box<SimplePrototype>> = proto.detach();
}

#[no_mangle]
pub unsafe extern "C" fn simple_onUnregister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().unwrap());
    simple_on_unregister(&fname);
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_foo(feature: *mut c_void, _adata: AppendData) -> FtString {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let ret = unsafe { (*simple).foo() };
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_bar(
    feature: *mut c_void,
    _adata: AppendData,
    a: FtInt,
    b: FtFloat,
) -> FtInt {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    unsafe { (*simple).bar(a, b) }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_goo(
    feature: *mut c_void,
    _adata: AppendData,
    a: FtDouble,
) -> FtDouble {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    unsafe { (*simple).goo(a) }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_doo(feature: *mut c_void, _adata: AppendData) {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    unsafe { (*simple).doo() }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_hoo(feature: *mut c_void, _adata: AppendData, a: FtString) {
    if a.is_null() {
        info!("Error: Received null pointer!");
        return;
    }
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let fs = unsafe { FeatureString::from_raw(a) };
    unsafe {
        (*simple).hoo(&fs);
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_chapter(
    feature: *mut c_void,
    _adata: AppendData,
    chap: *mut simple_Chapter_for_c,
) {
    if chap.is_null() {
        info!("wjf set_chapter() Received null pointer!");
    }
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    unsafe {
        let ptr = FeaturePtr::<simple_Chapter_for_c>::from_raw(chap);
        (*simple).set_chapter(simple_Chapter(ptr))
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_chapter(
    feature: *mut c_void,
    _adata: AppendData,
) -> *mut simple_Chapter_for_c {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let simple = unsafe { &*simple };
    let chap = simple.get_chapter();
    chap.map_or(ptr::null::<simple_Chapter_for_c>() as *mut _, |v| {
        v.0.clone().into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_chapter_array(
    feature: *mut c_void,
    _adata: AppendData,
    chap_array: *mut FtArray,
) {
    if chap_array.is_null() {
        info!("wjf set_chapter_array() Received null pointer!");
    }

    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let simple = unsafe { &mut *simple };
    unsafe {
        let chaps = FeatureReferenceArray::<simple_Chapter>::from_raw(chap_array);
        simple.set_chapter_array(chaps);
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_chapter_array(
    feature: *mut c_void,
    _adata: AppendData,
) -> *mut FtArray {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let chaps = unsafe { (*simple).get_chapter_array() };
    chaps.map_or(ptr::null::<FtArray>() as *mut _, |b| b.into_raw())
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_book(
    feature: *mut c_void,
    _adata: AppendData,
    book: *mut simple_Book_for_c,
) {
    if book.is_null() {
        info!("wjf set_book() Received null pointer!");
    } else {
        let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
        unsafe {
            let book = simple_Book::new(FeaturePtr::from_raw(book), FeatureInstance::new(feature));
            (*simple).set_book(book);
        };
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_book(
    feature: *mut c_void,
    _adata: AppendData,
) -> *mut simple_Book_for_c {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let book = unsafe { (*simple).get_book() };
    book.map_or(ptr::null::<simple_Book_for_c>() as *mut _, |b| {
        b.book.clone().into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_moo(
    feature: *mut c_void,
    _adata: AppendData,
    a: FtInt,
    id: FtCallbackId,
) {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let cb = MooCb::new(id, FeatureInstance::new(feature));
    unsafe { (*simple).moo(a, cb) }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_noo(
    feature: *mut c_void,
    _adata: AppendData,
    resolve: FtBool,
    id: FtPromiseId,
) {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let simple = unsafe { &mut *simple };
    let promise = unsafe { FeaturePromise::<FtIntPromise>::new(id, feature) };
    runtime::spawn(async move {
        match simple.noo(resolve).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_poo(
    feature: *mut c_void,
    _adata: AppendData,
    resolve: FtBool,
    id: FtPromiseId,
) {
    let simple = unsafe { feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    let simple = unsafe { &mut *simple };
    let promise = unsafe { FeaturePromise::<FeatureStringPromise>::new(id, feature) };
    runtime::spawn(async move {
        match simple.poo(resolve).await {
            Ok(v) => promise.resolve(v),
            Err(e) => promise.reject(e),
        }
    });
}

// interface related
#[no_mangle]
pub unsafe extern "C" fn simple_wrap_createDog(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    _type: FtInt,
) -> FeatureInterfaceHandle {
    let simple = unsafe { &*feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    simple.create_dog()
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_createAirplane(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FeatureInterfaceHandle {
    let simple = unsafe { &*feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    simple.create_airplane()
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_createPigeon(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FeatureInterfaceHandle {
    let simple = unsafe { &*feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    simple.create_pigeon()
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_createCat(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FeatureInterfaceHandle {
    let simple = unsafe { &*feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    simple.create_cat()
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_setAnimal(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    animal: FeatureInterfaceHandle,
) {
    let simple = unsafe { &*feature_glue::get_instance_data::<dyn Simple>(feature).unwrap() };
    simple.set_animal(animal)
}

// Animal interface dog vtable functions
#[no_mangle]
pub extern "C" fn simple_Animal_interface_dog_finalize(_feature: FeatureInstanceHandle) {}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_get_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let dog = unsafe { &*feature_glue::get_instance_data::<dyn Animal>(feature).unwrap() };
    let ret = dog.get_name();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_set_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    name: FtString,
) {
    let dog = unsafe { &*feature_glue::get_instance_data::<dyn Animal>(feature).unwrap() };
    let fname = unsafe { FeatureString::from_raw(name) };
    dog.set_name(&fname);
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_get_legCount(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtInt {
    let dog = unsafe { &*feature_glue::get_instance_data::<dyn Animal>(feature).unwrap() };
    dog.get_legCount()
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_eatFood(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    foods: *mut FtArray,
) -> FtInt {
    let dog = unsafe { &*feature_glue::get_instance_data::<dyn Animal>(feature).unwrap() };
    let foods = unsafe { FeaturePrimitiveArray::<FeatureString>::from_raw(foods) };
    dog.eatFood(&foods)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_run(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    distance: FtInt,
    destination: FtString,
) -> FtString {
    let dog = unsafe { &*feature_glue::get_instance_data::<dyn Animal>(feature).unwrap() };
    let destination = unsafe { FeatureString::from_raw(destination) };
    let ret = dog.run(distance, &destination);
    FeatureString::into_raw(ret)
}

// Flyable interface airplane vtable functions
#[no_mangle]
pub extern "C" fn simple_Flyable_interface_airplane_finalize(_feature: FeatureInstanceHandle) {}

#[no_mangle]
pub unsafe extern "C" fn simple_Flyable_interface_airplane_fly(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> *mut FtArray {
    let airplane = unsafe { &*feature_glue::get_instance_data::<dyn Flyable>(feature).unwrap() };
    let ret = airplane.fly();
    FeaturePrimitiveArray::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Flyable_interface_airplane_get_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let airplane = unsafe { &*feature_glue::get_instance_data::<dyn Flyable>(feature).unwrap() };
    let ret = airplane.get_breed();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Flyable_interface_airplane_set_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    breed: FtString,
) {
    let airplane = unsafe { &*feature_glue::get_instance_data::<dyn Flyable>(feature).unwrap() };
    let fbreed = unsafe { FeatureString::from_raw(breed) };
    airplane.set_breed(&fbreed);
}

// Bird interface pigeon vtable functions
#[no_mangle]
pub extern "C" fn simple_Bird_interface_pigeon_finalize(_feature: FeatureInstanceHandle) {}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    let ret = pigeon.get_name();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_set_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    name: FtString,
) {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    let fname = unsafe { FeatureString::from_raw(name) };
    pigeon.set_name(&fname);
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_legCount(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtInt {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    pigeon.get_legCount()
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_eatFood(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    foods: *mut FtArray,
) -> FtInt {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    let foods = unsafe { FeaturePrimitiveArray::<FeatureString>::from_raw(foods) };
    pigeon.eatFood(&foods)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_run(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    distance: FtInt,
    destination: FtString,
) -> FtString {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    let fdestination = unsafe { FeatureString::from_raw(destination) };
    let ret = pigeon.run(distance, &fdestination);
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_fly(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> *mut FtArray {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    let ret = pigeon.fly();
    FeaturePrimitiveArray::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    let ret = pigeon.get_breed();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_set_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    breed: FtString,
) {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    let fbreed = unsafe { FeatureString::from_raw(breed) };
    pigeon.set_breed(&fbreed);
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_weight(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtInt {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    pigeon.get_weight()
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_set_weight(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    weight: FtInt,
) {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    pigeon.set_weight(weight)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_walk(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
) {
    let pigeon = unsafe { &*feature_glue::get_instance_data::<dyn Bird>(feature).unwrap() };
    pigeon.walk(pid)
}
