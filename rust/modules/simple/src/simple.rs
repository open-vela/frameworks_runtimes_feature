use alloc::boxed::Box;
use alloc::sync::Arc;
use async_trait::async_trait;
use core::ffi::{c_int, c_void};
use core::{ffi::CStr, ptr};
use feature_frm::*;
use feature_macros::feature_struct;
use vdk::async_runtime::runtime;

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
    pub fn simple_emit_data_changed(handle: FeatureInstanceHandle, data: FtString);
    pub fn simple_emit_state_changed(handle: FeatureInstanceHandle, data: FtInt);
    // Interface constructors
    pub fn simple_createDog_instance(handle: FeatureInstanceHandle) -> FeatureInterfaceHandle;
    pub fn simple_createAirplane_instance(handle: FeatureInstanceHandle) -> FeatureInterfaceHandle;
    pub fn simple_createPigeon_instance(handle: FeatureInstanceHandle) -> FeatureInterfaceHandle;
}

pub fn create_dog_instance(instance: &FeatureInstance) -> FeatureInstance {
    let dog = unsafe { simple_createDog_instance(instance.as_handle()) };
    let ret = FeatureInstance::new(dog);
    unsafe { FeatureFreeInstanceHandle(dog) };
    ret
}

pub fn create_airplane_instance(instance: &FeatureInstance) -> FeatureInstance {
    let airplane = unsafe { simple_createAirplane_instance(instance.as_handle()) };
    let ret = FeatureInstance::new(airplane);
    unsafe { FeatureFreeInstanceHandle(airplane) };
    ret
}

pub fn create_pigeon_instance(instance: &FeatureInstance) -> FeatureInstance {
    let pigeon = unsafe { simple_createPigeon_instance(instance.as_handle()) };
    let ret = FeatureInstance::new(pigeon);
    unsafe { FeatureFreeInstanceHandle(pigeon) };
    ret
}

#[repr(C)]
#[derive(Clone)]
#[feature_struct(wrapper_struct = "Chapter")]
pub struct simple_Chapter {
    page_count: FtInt,
    title: FtString,
    is_end: FtBool,
}

impl Chapter {
    pub fn new() -> Self {
        Self {
            inner: FeaturePtr::new(),
        }
    }

    pub fn get_page_count(&self) -> c_int {
        self.page_count
    }

    pub fn set_page_count(&mut self, count: c_int) {
        self.page_count = count;
    }

    pub fn get_title(&self) -> Option<FeatureString> {
        unsafe { FeatureString::from_raw(self.title) }
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
#[feature_struct(wrapper_struct = "Book", with_instance = "true")]
pub struct simple_Book {
    book_name: FtString,
    chap_1: *mut simple_Chapter,
    chap_changed: FtCallbackId,
    book_info: FtJsonObject,
    chaps_info: *mut FtArray,
}

impl Book {
    pub fn new(inner: FeaturePtr<simple_Book>, instance: FeatureInstance) -> Self {
        Self { inner, instance }
    }

    pub fn get_book_name(&self) -> Option<FeatureString> {
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

    pub fn get_chap_1(&self) -> Option<Chapter> {
        unsafe { Chapter::from_raw(self.chap_1) }
    }

    pub fn set_chap_1(&mut self, chap_1: Chapter) {
        if !self.chap_1.is_null() {
            //  free old data
            unsafe {
                FeatureFreeValue(self.chap_1 as *mut c_void);
            }
        }
        self.chap_1 = chap_1.into_raw()
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

    pub fn get_book_info(&self) -> Option<FeatureJsonObject> {
        unsafe { FeatureJsonObject::from_raw(self.book_info) }
    }

    pub fn set_book_info(&mut self, book_info: FeatureJsonObject) {
        if !self.book_info.is_null() {
            //  free old data
            unsafe {
                FeatureFreeValue(self.book_info as *mut c_void);
            }
        }
        self.book_info = FeatureJsonObject::into_raw(book_info);
    }

    pub fn get_chaps_info(&self) -> Option<FeatureReferenceArray<FeatureJsonObject>> {
        unsafe { FeatureReferenceArray::<FeatureJsonObject>::from_raw(self.chaps_info) }
    }

    pub fn set_chaps_info(&mut self, chaps_info: FeatureReferenceArray<FeatureJsonObject>) {
        if !self.chaps_info.is_null() {
            //  free old data
            unsafe {
                FeatureFreeValue(self.chaps_info as *mut c_void);
            }
        }
        self.chaps_info = chaps_info.into_raw();
    }
}

impl Drop for Book {
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

pub struct DataChangedEvent {
    ev: Arc<FeatureEvent>,
}

impl DataChangedEvent {
    pub(crate) fn new(id: FtEventId, instance: FeatureInstance) -> Self {
        Self {
            ev: Arc::new(FeatureEvent::new(id, instance)),
        }
    }

    pub fn emit(&self, data: FeatureString) {
        unsafe {
            // must clone the Arc<FeatureEvent> and move it to the closure
            // to prevent the FeatureEvent from being dropped before the closure is called.
            let ev = self.ev.clone();
            self.ev.post(move || {
                simple_emit_data_changed(ev.handle(), data.as_ptr());
            });
        }
    }
}

pub struct StateChangedEvent {
    ev: Arc<FeatureEvent>,
}

impl StateChangedEvent {
    pub(crate) fn new(id: FtEventId, instance: FeatureInstance) -> Self {
        Self {
            ev: Arc::new(FeatureEvent::new(id, instance)),
        }
    }

    pub fn emit(&self, state: FtInt) {
        unsafe {
            let ev = self.ev.clone();
            self.ev.post(move || {
                simple_emit_state_changed(ev.handle(), state);
            });
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
    fn hoo(&mut self, a: &Option<FeatureString>);
    fn set_book(&mut self, book: Option<Book>);
    fn get_book(&mut self) -> Option<Book>;
    fn set_chapter(&mut self, chap: Option<Chapter>);
    fn get_chapter(&self) -> Option<Chapter>;
    fn set_chapter_array(&mut self, chap_array: Option<FeatureReferenceArray<Chapter>>);
    fn get_chapter_array(&mut self) -> Option<FeatureReferenceArray<Chapter>>;
    fn moo(&mut self, a: i32, cb: MooCb);
    async fn noo(&mut self, resolve: FtBool) -> Result<FtInt, PromiseError>;
    async fn poo(&mut self, resolve: FtBool) -> Result<FeatureString, PromiseError>;
    fn create_dog(&self) -> Option<FeatureInstance>;
    fn create_airplane(&self) -> Option<FeatureInstance>;
    fn create_pigeon(&self) -> Option<FeatureInstance>;
    fn create_cat(&self) -> Option<FeatureInstance>;
    fn set_animal(&self, animal: FeatureInstance);
    fn invoke_event(&self, event_name: &Option<FeatureString>);
    fn set_buffer(&self, buffer: Option<FeatureArrayBuffer>);
    fn get_buffer_copy(&self) -> FeatureArrayBuffer;
    fn get_buffer_no_copy(&self) -> FeatureArrayBuffer;
    fn set_buffer_array(&self, ab_array: Option<FeatureReferenceArray<FeatureArrayBuffer>>);
    fn get_buffer_array_copy(&self) -> Option<FeatureReferenceArray<FeatureArrayBuffer>>;
    fn get_buffer_array_no_copy(&self) -> Option<FeatureReferenceArray<FeatureArrayBuffer>>;
}

// Interface trait
pub trait Animal: FeatureInstanceTrait {
    fn get_name(&self) -> FeatureString;
    fn set_name(&mut self, name: Option<FeatureString>);
    fn get_leg_count(&self) -> FtInt;
    fn eat_foods(&self, foods: &Option<FeaturePrimitiveArray<FeatureString>>) -> FtInt;
    fn run(&self, distance: FtInt, destination: &Option<FeatureString>) -> FeatureString;
}

pub trait Flyable: FeatureInstanceTrait {
    fn fly(&self) -> FeaturePrimitiveArray<FeatureString>;
    fn get_breed(&self) -> FeatureString;
    fn set_breed(&mut self, breed: Option<FeatureString>);
}

pub trait Bird: Animal + Flyable + FeatureInstanceTrait {
    fn get_weight(&self) -> FtInt;
    fn set_weight(&mut self, weight: FtInt);
    fn walk(&self, pid: FtPromiseId);
}

#[no_mangle]
pub unsafe extern "C" fn simple_onRegister(feature_name: FtString) {
    let name = unsafe { CStr::from_ptr(feature_name) };
    let fname = FeatureString::new(name.to_str().expect("Failed to get feature name"));
    simple_on_register(&fname);
}

#[no_mangle]
pub extern "C" fn simple_onCreate(
    ctx: FeatureRuntimeContextHandle,
    proto_handle: FeatureProtoHandle,
) {
    let proto = FeaturePrototype::new(proto_handle);
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
    let fname = FeatureString::new(name.to_str().expect("Failed to get feature name"));
    simple_on_unregister(&fname);
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_foo(feature: *mut c_void, _adata: AppendData) -> FtString {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
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
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    unsafe { (*simple).bar(a, b) }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_goo(
    feature: *mut c_void,
    _adata: AppendData,
    a: FtDouble,
) -> FtDouble {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    unsafe { (*simple).goo(a) }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_doo(feature: *mut c_void, _adata: AppendData) {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    unsafe { (*simple).doo() }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_hoo(feature: *mut c_void, _adata: AppendData, a: FtString) {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let fs = unsafe { FeatureString::from_raw(a) };
    unsafe {
        (*simple).hoo(&fs);
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_chapter(
    feature: *mut c_void,
    _adata: AppendData,
    chap: *mut simple_Chapter,
) {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    unsafe { (*simple).set_chapter(Chapter::from_raw(chap)) }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_chapter(
    feature: *mut c_void,
    _adata: AppendData,
) -> *mut simple_Chapter {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let simple = unsafe { &*simple };
    let chap = simple.get_chapter();
    chap.map_or(ptr::null::<simple_Chapter>() as *mut _, |v| {
        v.inner.clone().into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_chapter_array(
    feature: *mut c_void,
    _adata: AppendData,
    chap_array: *mut FtArray,
) {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let simple = unsafe { &mut *simple };
    unsafe {
        let chaps = FeatureReferenceArray::<Chapter>::from_raw(chap_array);
        simple.set_chapter_array(chaps);
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_chapter_array(
    feature: *mut c_void,
    _adata: AppendData,
) -> *mut FtArray {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let chaps = unsafe { (*simple).get_chapter_array() };
    chaps.map_or(ptr::null::<FtArray>() as *mut _, |b| b.into_raw())
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_book(
    feature: *mut c_void,
    _adata: AppendData,
    book: *mut simple_Book,
) {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let book = unsafe { FeaturePtr::from_raw(book) }
        .map(|ptr| Book::new(ptr, FeatureInstance::new(feature)));
    (*simple).set_book(book);
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_book(
    feature: *mut c_void,
    _adata: AppendData,
) -> *mut simple_Book {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let book = unsafe { (*simple).get_book() };
    book.map_or(ptr::null::<Book>() as *mut _, |b| {
        b.inner.clone().into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_moo(
    feature: *mut c_void,
    _adata: AppendData,
    a: FtInt,
    id: FtCallbackId,
) {
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
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
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
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
    let simple = unsafe {
        feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let simple = unsafe { &mut *simple };
    let promise = unsafe { FeaturePromise::<FtStringPromise>::new(id, feature) };
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
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    if let Some(dog) = simple.create_dog() {
        dog.as_handle()
    } else {
        ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_createAirplane(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FeatureInterfaceHandle {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    if let Some(airplane) = simple.create_airplane() {
        airplane.as_handle()
    } else {
        ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_createPigeon(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FeatureInterfaceHandle {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    if let Some(pigeon) = simple.create_pigeon() {
        pigeon.as_handle()
    } else {
        ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_createCat(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FeatureInterfaceHandle {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    if let Some(cat) = simple.create_cat() {
        cat.as_handle()
    } else {
        ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_setAnimal(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    animal: FeatureInterfaceHandle,
) {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    simple.set_animal(FeatureInstance::new(animal))
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_invoke_event(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    event_name: FtString,
) {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let name = unsafe { FeatureString::from_raw(event_name) };
    simple.invoke_event(&name);
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_buffer(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    buff: FtArrayBuffer,
) {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let abuf = unsafe { FeatureArrayBuffer::from_raw(buff) };
    simple.set_buffer(abuf);
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_buffer_copy(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtArrayBuffer {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let abuf = simple.get_buffer_copy();
    abuf.into_raw()
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_buffer_no_copy(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtArrayBuffer {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let abuf = simple.get_buffer_no_copy();
    abuf.into_raw()
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_set_buffer_array(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    buff_array: *mut FtArray,
) {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let ab_array = unsafe { FeatureReferenceArray::<FeatureArrayBuffer>::from_raw(buff_array) };
    simple.set_buffer_array(ab_array);
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_buffer_array_copy(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> *mut FtArray {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let ret = simple.get_buffer_array_copy();
    if let Some(wrapper) = ret {
        wrapper.into_raw()
    } else {
        ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn simple_wrap_get_buffer_array_no_copy(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> *mut FtArray {
    let simple = unsafe {
        &*feature_glue::get_instance_data::<dyn Simple>(feature)
            .expect("Failed to get impl for 'Simple' trait")
    };
    let ret = simple.get_buffer_array_no_copy();
    if let Some(wrapper) = ret {
        wrapper.into_raw()
    } else {
        ptr::null_mut()
    }
}

// Animal interface dog vtable functions
#[no_mangle]
pub extern "C" fn simple_Animal_interface_dog_finalize(feature: FeatureInstanceHandle) {
    let instance = FeatureInstance::new(feature);
    let _: Option<Box<dyn Animal>> = instance.detach();
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_get_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let dog = unsafe {
        &*feature_glue::get_instance_data::<dyn Animal>(feature)
            .expect("Failed to get impl for 'Animal' trait")
    };
    let ret = dog.get_name();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_set_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    name: FtString,
) {
    let dog = unsafe {
        feature_glue::get_instance_data::<dyn Animal>(feature)
            .expect("Failed to get impl for 'Animal' trait")
    };
    let fname = unsafe { FeatureString::from_raw(name) };
    (*dog).set_name(fname);
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_get_legCount(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtInt {
    let dog = unsafe {
        &*feature_glue::get_instance_data::<dyn Animal>(feature)
            .expect("Failed to get impl for 'Animal' trait")
    };
    dog.get_leg_count()
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_eatFoods(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    foods: *mut FtArray,
) -> FtInt {
    let dog = unsafe {
        &*feature_glue::get_instance_data::<dyn Animal>(feature)
            .expect("Failed to get impl for 'Animal' trait")
    };
    let foods = unsafe { FeaturePrimitiveArray::<FeatureString>::from_raw(foods) };
    dog.eat_foods(&foods)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Animal_interface_dog_run(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    distance: FtInt,
    destination: FtString,
) -> FtString {
    let dog = unsafe {
        &*feature_glue::get_instance_data::<dyn Animal>(feature)
            .expect("Failed to get impl for 'Animal' trait")
    };
    let destination = unsafe { FeatureString::from_raw(destination) };
    let ret = dog.run(distance, &destination);
    FeatureString::into_raw(ret)
}

// Flyable interface airplane vtable functions
#[no_mangle]
pub extern "C" fn simple_Flyable_interface_airplane_finalize(feature: FeatureInstanceHandle) {
    let instance = FeatureInstance::new(feature);
    let _: Option<Box<dyn Flyable>> = instance.detach();
}

#[no_mangle]
pub unsafe extern "C" fn simple_Flyable_interface_airplane_fly(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> *mut FtArray {
    let airplane = unsafe {
        &*feature_glue::get_instance_data::<dyn Flyable>(feature)
            .expect("Failed to get impl for 'Flyable' trait")
    };
    let ret = airplane.fly();
    FeaturePrimitiveArray::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Flyable_interface_airplane_get_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let airplane = unsafe {
        &*feature_glue::get_instance_data::<dyn Flyable>(feature)
            .expect("Failed to get impl for 'Flyable' trait")
    };
    let ret = airplane.get_breed();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Flyable_interface_airplane_set_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    breed: FtString,
) {
    let airplane = unsafe {
        feature_glue::get_instance_data::<dyn Flyable>(feature)
            .expect("Failed to get impl for 'Flyable' trait")
    };
    let fbreed = unsafe { FeatureString::from_raw(breed) };
    (*airplane).set_breed(fbreed);
}

// Bird interface pigeon vtable functions
#[no_mangle]
pub extern "C" fn simple_Bird_interface_pigeon_finalize(feature: FeatureInstanceHandle) {
    let instance = FeatureInstance::new(feature);
    let _: Option<Box<dyn Bird>> = instance.detach();
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    let ret = pigeon.get_name();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_set_name(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    name: FtString,
) {
    let pigeon = unsafe {
        feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    let fname = unsafe { FeatureString::from_raw(name) };
    (*pigeon).set_name(fname);
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_legCount(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtInt {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    pigeon.get_leg_count()
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_eatFoods(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    foods: *mut FtArray,
) -> FtInt {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    let foods = unsafe { FeaturePrimitiveArray::<FeatureString>::from_raw(foods) };
    pigeon.eat_foods(&foods)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_run(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    distance: FtInt,
    destination: FtString,
) -> FtString {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    let fdestination = unsafe { FeatureString::from_raw(destination) };
    let ret = pigeon.run(distance, &fdestination);
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_fly(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> *mut FtArray {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    let ret = pigeon.fly();
    FeaturePrimitiveArray::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtString {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    let ret = pigeon.get_breed();
    FeatureString::into_raw(ret)
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_set_breed(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    breed: FtString,
) {
    let pigeon = unsafe {
        feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    let fbreed = unsafe { FeatureString::from_raw(breed) };
    (*pigeon).set_breed(fbreed);
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_get_weight(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
) -> FtInt {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    pigeon.get_weight()
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_set_weight(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    weight: FtInt,
) {
    let pigeon = unsafe {
        feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    (*pigeon).set_weight(weight);
}

#[no_mangle]
pub unsafe extern "C" fn simple_Bird_interface_pigeon_walk(
    feature: FeatureInstanceHandle,
    _adata: AppendData,
    pid: FtPromiseId,
) {
    let pigeon = unsafe {
        &*feature_glue::get_instance_data::<dyn Bird>(feature)
            .expect("Failed to get impl for 'Bird' trait")
    };
    pigeon.walk(pid)
}
