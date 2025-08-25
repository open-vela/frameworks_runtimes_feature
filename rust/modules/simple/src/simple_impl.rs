use crate::simple::*;
use alloc::{boxed::Box, string::String};
use async_trait::async_trait;
use core::{ptr, time::Duration};
use feature_frm::*;
use feature_macros::feature_instance;
use vdk::async_runtime::time;
use vdk::log::info;

pub fn simple_on_register(_name: &FeatureString) {}

pub fn simple_on_create(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub fn simple_on_required(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {}

pub fn simple_on_detached(_ctx: FeatureRuntimeContext, _instance: FeatureInstance) {}

pub fn simple_on_destroy(_ctx: FeatureRuntimeContext, _proto: FeaturePrototype) {}

pub fn simple_on_unregister(_name: &FeatureString) {}

pub struct SimplePrototype {
    pub proto: FeaturePrototype,
    pub str: String,
}

impl SimplePrototype {
    pub(crate) fn new(proto: FeaturePrototype) -> Self {
        SimplePrototype {
            proto,
            str: String::from("SimpleImpl"),
        }
    }
}

#[feature_instance(name = "Simple")]
pub struct SimpleImpl {
    instance: FeatureInstance,
    chapter: Option<simple_Chapter>,
    book: Option<simple_Book>,
    chap_changed: Option<ChapterChangedCb>,
}

// function implementation
impl SimpleImpl {
    pub(crate) fn new(instance: FeatureInstance) -> Self {
        SimpleImpl {
            instance,
            chapter: None,
            book: None,
            chap_changed: None,
        }
    }

    pub fn get_prototype(&self) -> Option<&SimplePrototype> {
        self.instance.get_prototype::<SimplePrototype>()
    }
}

#[async_trait]
impl Simple for SimpleImpl {
    fn foo(&mut self) -> FeatureString {
        info!("wjf foo Called from C");
        FeatureString::new("foo Called")
    }

    fn bar(&mut self, a: FtInt, b: FtFloat) -> FtInt {
        info!("wjf bar Called from C, a: {}, b: {}", a, b);
        a + b as FtInt
    }

    fn goo(&mut self, a: FtDouble) -> FtDouble {
        info!("wjf goo Called from C, a: {}", a);
        a + 1.0 as FtDouble
    }

    fn doo(&mut self) {
        info!("wjf doo Called from C");
    }

    fn hoo(&mut self, a: &FeatureString) {
        info!("wjf hoo Called from C, a: \"{}\"", a.as_str());
        let proto = self.get_prototype().unwrap();
        info!("proto.str: {}", proto.str);
        let _mgr = self.instance.get_manager();

        if let Some(version) = proto.proto.get_package_version() {
            info!("package version: {}", version);
        } else {
            info!("No package version available");
        }

        if let Some(name) = proto.proto.get_package_name() {
            info!("package name: {}", name);
        } else {
            info!("No package name available");
        }
    }

    fn set_book(&mut self, mut book: simple_Book) {
        let chap_title = book.get_chap_1().get_title().unwrap();
        info!(
            "set_book called from C, book_name: {}, chap_title: {}",
            book.get_book_name(),
            chap_title
        );
        self.chap_changed = book.take_chap_changed();
        self.book = Some(book);
    }

    fn get_book(&mut self) -> Option<simple_Book> {
        info!("wjf get_book Called from C");
        self.book.clone()
    }

    fn set_chapter(&mut self, chap: simple_Chapter) {
        info!(
            "set_chapter called from C, page_count:{}",
            chap.get_page_count()
        );
        if let Some(cb) = &self.chap_changed {
            let chap_title = chap.get_title().unwrap();
            cb.invoke(1, chap_title);
        } else {
            info!("No chap_changed callback available");
        }
        self.chapter = Some(chap);
    }

    fn get_chapter(&self) -> Option<simple_Chapter> {
        self.chapter.clone()
    }

    fn set_chapter_array(&mut self, chap_array: FeatureReferenceArray<simple_Chapter>) {
        info!("wjf set_chapter_array Called from C");
        for i in 0..chap_array.len() {
            let item: simple_Chapter = chap_array.get(i).unwrap();
            info!(
                "wjf i: {}, chap.page_count: {}, chap.title: {}, chap.is_end: {}",
                i,
                item.get_page_count(),
                item.get_title().unwrap(),
                item.get_is_end()
            );
        }
    }

    fn get_chapter_array(&mut self) -> Option<FeatureReferenceArray<simple_Chapter>> {
        info!("wjf get_chapter_array Called from C");
        let mut ret = FeatureReferenceArray::<simple_Chapter>::new(2);
        let mut chap1 = simple_Chapter::new();
        chap1.set_title(FeatureString::new("chapter 1"));
        chap1.set_is_end(false);
        chap1.set_page_count(100);
        let mut chap2 = simple_Chapter::new();
        chap2.set_title(FeatureString::new("chapter 2"));
        chap2.set_is_end(true);
        chap2.set_page_count(300);
        ret.append(chap1);
        ret.append(chap2);
        Some(ret)
    }

    fn moo(&mut self, a: i32, cb: MooCb) {
        info!("wjf moo Called from C, a: {}", a);
        let bs = FeatureString::new("moo called");
        cb.invoke(a, bs, 1.34);
    }

    async fn noo(&mut self, resolve: FtBool) -> Result<FtInt, PromiseError> {
        info!("wjf noo Called from C, resolve: {}", resolve);

        time::sleep(Duration::from_millis(100)).await; // simulate async delay
        if resolve {
            Ok(5)
        } else {
            Err(PromiseError::new(400, "noo rejected"))
        }
    }

    async fn poo(&mut self, resolve: FtBool) -> Result<FeatureString, PromiseError> {
        info!("wjf poo Called from C, resolve: {}", resolve);

        time::sleep(Duration::from_millis(100)).await; // simulate async delay
        if resolve {
            Ok(FeatureString::new("poo resolved!"))
        } else {
            Err(PromiseError::new(500, "poo rejected"))
        }
    }

    fn create_dog(&self) -> FeatureInterfaceHandle {
        info!("wjf create_dog Called from C");

        let handle = createDog_instance(&self.instance);
        let instance = FeatureInstance::new(handle);
        let boxed: Box<dyn Animal> = Box::new(Dog::new(instance.clone()));
        instance.attach(boxed);

        handle
    }

    fn create_airplane(&self) -> FeatureInterfaceHandle {
        info!("wjf create_airplane Called from C");
        let handle = createAirplane_instance(&self.instance);
        let instance = FeatureInstance::new(handle);
        let boxed: Box<dyn Flyable> = Box::new(Airplane::new(instance.clone()));
        instance.attach(boxed);
        handle
    }

    fn create_pigeon(&self) -> FeatureInterfaceHandle {
        info!("wjf create_pigeon Called from C");
        let handle = createPigeon_instance(&self.instance);
        let instance = FeatureInstance::new(handle);
        let boxed: Box<dyn Bird> = Box::new(Pigeon::new(instance.clone()));
        instance.attach(boxed);
        handle
    }

    fn create_cat(&self) -> FeatureInterfaceHandle {
        info!("wjf create_cat Called from C");
        ptr::null_mut() as FeatureInterfaceHandle
    }

    fn set_animal(&self, _animal: FeatureInterfaceHandle) {
        info!("wjf set_animal Called from C");
    }
}

impl Drop for SimpleImpl {
    fn drop(&mut self) {
        info!("wjf SimpleImpl droped");
    }
}

#[feature_instance(name = "Animal")]
pub struct Dog {
    instance: FeatureInstance,
}

// function implementation
impl Dog {
    fn new(instance: FeatureInstance) -> Self {
        Dog { instance }
    }
}

impl Animal for Dog {
    fn get_name(&self) -> FeatureString {
        info!("wjf dog get_name Called from C");
        FeatureString::new("Puppy")
    }

    fn set_name(&self, _name: &FeatureString) {
        info!("wjf dog set_name Called from C");
    }

    fn get_legCount(&self) -> FtInt {
        info!("wjf dog get_legCount Called from C");
        4
    }

    fn eatFood(&self, foods: &FeaturePrimitiveArray<FeatureString>) -> FtInt {
        info!("wjf dog eatFood Called from C");
        for i in 0..foods.len() {
            let item = foods.get(i).unwrap();
            info!("{} dog food: {}", i, *item);
        }
        foods.len() as i32
    }

    fn run(&self, _distance: FtInt, _destination: &FeatureString) -> FeatureString {
        info!("wjf dog run Called from C");
        FeatureString::new("dog_run")
    }
}

impl Drop for Dog {
    fn drop(&mut self) {
        info!("wjf Dog droped");
    }
}

#[feature_instance(name = "Flyable")]
pub struct Airplane {
    instance: FeatureInstance,
}

// function implementation
impl Airplane {
    fn new(instance: FeatureInstance) -> Self {
        Airplane { instance }
    }
}

impl Flyable for Airplane {
    fn fly(&self) -> FeaturePrimitiveArray<FeatureString> {
        info!("wjf airplane fly Called from C");
        let mut ret = FeaturePrimitiveArray::<FeatureString>::new(4);
        ret.append_string(FeatureString::from("airplane fly1"));
        ret.append_string(FeatureString::from("airplane fly2"));
        ret.append_string(FeatureString::from("airplane fly3"));
        ret.append_string(FeatureString::from("airplane fly4"));
        ret
    }

    fn get_breed(&self) -> FeatureString {
        info!("wjf airplane get_breed Called from C");
        FeatureString::new("airplane_get_breed")
    }

    fn set_breed(&self, _breed: &FeatureString) {
        info!("wjf airplane set_breed Called from C");
    }
}

impl Drop for Airplane {
    fn drop(&mut self) {
        info!("wjf Airplane droped");
    }
}

#[feature_instance(name = "Bird")]
pub struct Pigeon {
    instance: FeatureInstance,
}

// function implementation
impl Pigeon {
    fn new(instance: FeatureInstance) -> Self {
        Pigeon { instance }
    }
}

impl Animal for Pigeon {
    fn get_name(&self) -> FeatureString {
        info!("wjf pigeon get_name Called from C");
        FeatureString::new("pigeon_get_name")
    }

    fn set_name(&self, _name: &FeatureString) {
        info!("wjf pigeon set_name Called from C");
    }

    fn get_legCount(&self) -> FtInt {
        info!("wjf pigeon get_legCount Called from C");
        5
    }

    fn eatFood(&self, foods: &FeaturePrimitiveArray<FeatureString>) -> FtInt {
        info!("wjf pigeon eatFood Called from C");
        for i in 0..foods.len() {
            let item = foods.get(i).unwrap();
            info!("{} pigeon food: {}", i, *item);
        }
        foods.len() as i32
    }

    fn run(&self, _distance: FtInt, _destination: &FeatureString) -> FeatureString {
        info!("wjf pigeon run Called from C");
        FeatureString::new("pigeon_run")
    }
}

impl Flyable for Pigeon {
    fn fly(&self) -> FeaturePrimitiveArray<FeatureString> {
        info!("wjf pigeon fly Called from C");
        let mut ret = FeaturePrimitiveArray::<FeatureString>::new(3);
        ret.append_string(FeatureString::from("pigeon fly5"));
        ret.append_string(FeatureString::from("pigeon fly6"));
        ret.append_string(FeatureString::from("pigeon fly7"));
        ret
    }

    fn get_breed(&self) -> FeatureString {
        info!("wjf pigeon get_breed Called from C");
        FeatureString::new("pigeon_get_breed")
    }

    fn set_breed(&self, _breed: &FeatureString) {
        info!("wjf pigeon set_breed Called from C");
    }
}

impl Bird for Pigeon {
    fn get_weight(&self) -> FtInt {
        info!("wjf pigeon get_weight Called from C");
        7
    }

    fn set_weight(&self, _weight: FtInt) {
        info!("wjf pigeon set_weight Called from C");
    }

    fn walk(&self, _pid: FtPromiseId) {
        info!("wjf pigeon walk Called from C");
    }
}

impl Drop for Pigeon {
    fn drop(&mut self) {
        info!("wjf Pigeon droped");
    }
}

/*
#[feature_method_async("simple_wrap_foo")]
pub async fn foo(_feature: *mut c_void) -> *const i8 {
    info!("wjf foo Called from C");
    b"world\0".as_ptr() as *const i8
}

#[feature_method_async("simple_wrap_bar")]
pub async fn bar(_feature: *mut c_void, a: c_int, b: c_float) -> c_int {
    info!("wjf bar Called from C, a: {}, b: {}", a.to_string(), b);
    a + 1
}

#[feature_method_async("simple_wrap_goo")]
pub async fn goo(_feature: *mut c_void, a: c_double) -> c_double {
    info!("wjf goo Called from C, a: {}", a);
    a + 5.0
}

#[feature_method_async("simple_wrap_doo")]
pub async fn doo(_feature: *mut c_void) {
    info!("wjf doo Called from C")
}
*/
