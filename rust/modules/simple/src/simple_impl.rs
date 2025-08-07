use crate::simple::*;
use async_trait::async_trait;
use feature_frm::*;
use feature_macros::feature_instance;
use feature_sys::*;
use std::time::Duration;
use vdk::async_runtime::time;

pub fn simple_on_register(name: &FeatureString) {
    println!("SimpleImpl on_register: {}", name.as_str())
}

pub fn simple_on_create(_ctx: FeatureRuntimeContext, proto_handle: FeatureProtoHandle) {
    println!("SimpleImpl on_create");
    unsafe { std::env::set_var("RUST_BACKTRACE", "full") };
    FeaturePrototype::attach(proto_handle, Box::new(SimplePrototype::new(proto_handle)))
}

pub fn simple_on_required(_ctx: FeatureRuntimeContext, instance_handle: FeatureInstanceHandle) {
    println!("SimpleImpl on_required");
    let boxed = Box::new(SimpleImpl::new(instance_handle)) as Box<dyn Simple>;
    FeatureInstance::attach(instance_handle, boxed);
}

pub fn simple_on_detached(_ctx: FeatureRuntimeContext, instance_handle: FeatureInstanceHandle) {
    println!("SimpleImpl on_detached");
    let _ = FeatureInstance::detach::<SimpleImpl>(instance_handle);
}

pub fn simple_on_destroy(_ctx: FeatureRuntimeContext, proto_handle: FeatureProtoHandle) {
    println!("SimpleImpl on_destroy");
    let _ = FeaturePrototype::detach::<SimplePrototype>(proto_handle);
}

pub fn simple_on_unregister(name: &FeatureString) {
    println!("SimpleImpl on_unregister: {}", name.as_str());
}

pub struct SimplePrototype {
    pub proto: FeaturePrototype,
    pub str: String,
}

impl SimplePrototype {
    fn new(handle: FeatureProtoHandle) -> Self {
        SimplePrototype {
            proto: FeaturePrototype::new(handle),
            str: String::from("SimpleImpl"),
        }
    }
}

#[feature_instance(name = "Simple")]
pub struct SimpleImpl {
    chapter: Option<simple_Chapter>,
    book: Option<simple_Book>,
}

impl FeatureInstanceTrait for SimpleImpl {}

// function implementation
impl SimpleImpl {
    fn new(handle: FeatureInstanceHandle) -> Self {
        let instance = FeatureInstance::new(handle);
        SimpleImpl {
            instance: instance.clone(),
            chapter: None,
            book: None,
        }
    }

    pub fn get_prototype(&self) -> Option<*mut SimplePrototype> {
        self.instance.get_prototype::<SimplePrototype>()
    }
}

#[async_trait]
impl Simple for SimpleImpl {
    fn foo(&mut self) -> FeatureString {
        println!("wjf foo Called from C");
        FeatureString::new("foo Called")
    }

    fn bar(&mut self, a: FtInt, b: FtFloat) -> FtInt {
        println!("wjf bar Called from C, a: {}, b: {}", a, b);
        a + b as FtInt
    }

    fn goo(&mut self, a: FtDouble) -> FtDouble {
        println!("wjf goo Called from C, a: {}", a);
        a + 1.0 as FtDouble
    }

    fn doo(&mut self) {
        println!("wjf doo Called from C");
    }

    fn hoo(&mut self, a: &FeatureString) {
        println!("wjf hoo Called from C, a: \"{}\"", a.as_str());
        let proto = unsafe { &*self.get_prototype().unwrap() };
        println!("proto.str: {}", proto.str);
        let _mgr = self.instance.get_manager();

        if let Some(version) = proto.proto.get_package_version() {
            println!("package version: {}", version);
        } else {
            println!("No package version available");
        }

        if let Some(name) = proto.proto.get_package_name() {
            println!("package name: {}", name);
        } else {
            println!("No package name available");
        }
    }

    fn set_book(&mut self, book: simple_Book) {
        self.book = Some(book);
    }

    fn get_book(&mut self) -> Option<simple_Book> {
        self.book.clone()
    }

    fn set_chapter(&mut self, chap: simple_Chapter) {
        println!("set_chapter called from C, title:{}", chap.get_page_count());
        self.chapter = Some(chap);
    }

    fn get_chapter(&self) -> Option<simple_Chapter> {
        self.chapter.clone()
    }

    fn set_chapter_array(&mut self, chap_array: FeatureReferenceArray<simple_Chapter>) {
        println!("wjf set_chapter_array Called from C");
        for i in 0..chap_array.len() {
            let item: simple_Chapter = chap_array.get(i).unwrap();
            println!(
                "wjf i: {}, chap.page_count: {}, chap.title: {}, chap.is_end: {}",
                i,
                item.get_page_count(),
                item.get_title().unwrap(),
                item.get_is_end()
            );
        }
    }

    fn get_chapter_array(&mut self) -> Option<FeatureReferenceArray<simple_Chapter>> {
        println!("wjf get_chapter_array Called from C");
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

    fn moo(&mut self, a: i32, cb: moo_cb) {
        println!("wjf moo Called from C, a: {}", a);
        let bs = FeatureString::new("moo called");
        cb.invoke(a, &bs, 1.34);
    }

    async fn noo(&mut self, resolve: FtBool) -> Result<FtInt, PromiseError> {
        println!("wjf noo Called from C, resolve: {}", resolve);

        time::sleep(Duration::from_millis(100)).await; // simulate async delay
        if resolve {
            Ok(5)
        } else {
            Err(PromiseError::new(400, "noo rejected"))
        }
    }

    async fn poo(&mut self, resolve: FtBool) -> Result<FeatureString, PromiseError> {
        println!("wjf poo Called from C, resolve: {}", resolve);

        time::sleep(Duration::from_millis(100)).await; // simulate async delay
        if resolve {
            Ok(FeatureString::new("poo resolved!"))
        } else {
            Err(PromiseError::new(500, "poo rejected"))
        }
    }

    fn create_dog(&self) -> FeatureInterfaceHandle {
        println!("wjf create_dog Called from C");
        let handle = createDog_instance(&self.instance);
        let boxed: Box<dyn Animal> = Box::new(Dog::new(handle));
        FeatureInstance::attach(handle, boxed);
        handle
    }

    fn create_airplane(&self) -> FeatureInterfaceHandle {
        println!("wjf create_airplane Called from C");
        let handle = createAirplane_instance(&self.instance);
        let boxed: Box<dyn Flyable> = Box::new(Airplane::new(handle));
        FeatureInstance::attach(handle, boxed);
        handle
    }

    fn create_pigeon(&self) -> FeatureInterfaceHandle {
        println!("wjf create_pigeon Called from C");
        let handle = createPigeon_instance(&self.instance);
        let boxed: Box<dyn Bird> = Box::new(Pigeon::new(handle));
        FeatureInstance::attach(handle, boxed);
        handle
    }

    fn create_cat(&self) -> FeatureInterfaceHandle {
        println!("wjf create_cat Called from C");
        std::ptr::null_mut() as FeatureInterfaceHandle
    }

    fn set_animal(&self, _animal: FeatureInterfaceHandle) {
        println!("wjf set_animal Called from C");
    }
}

impl Drop for SimpleImpl {
    fn drop(&mut self) {
        println!("wjf SimpleImpl droped");
    }
}

#[feature_instance(name = "Animal")]
pub struct Dog {}

// function implementation
impl Dog {
    fn new(handle: FeatureInstanceHandle) -> Self {
        Dog {
            instance: FeatureInstance::new(handle),
        }
    }
}

impl FeatureInstanceTrait for Dog {}

impl Animal for Dog {
    fn get_name(&self) -> FeatureString {
        println!("wjf dog get_name Called from C");
        FeatureString::new("Puppy")
    }

    fn set_name(&self, _name: &FeatureString) {
        println!("wjf dog set_name Called from C");
    }

    fn get_legCount(&self) -> FtInt {
        println!("wjf dog get_legCount Called from C");
        4
    }

    fn eatFood(&self, foods: &FeaturePrimitiveArray<FeatureString>) -> FtInt {
        println!("wjf dog eatFood Called from C");
        for i in 0..foods.len() {
            let item = foods.get(i).unwrap();
            println!("{} dog food: {}", i, *item);
        }
        foods.len() as i32
    }

    fn run(&self, _distance: FtInt, _destination: &FeatureString) -> FeatureString {
        println!("wjf dog run Called from C");
        FeatureString::new("dog_run")
    }
}

impl Drop for Dog {
    fn drop(&mut self) {
        println!("wjf Dog droped");
    }
}

#[feature_instance(name = "Flyable")]
pub struct Airplane {}

// function implementation
impl Airplane {
    fn new(handle: FeatureInstanceHandle) -> Self {
        Airplane {
            instance: FeatureInstance::new(handle),
        }
    }
}

impl FeatureInstanceTrait for Airplane {}

impl Flyable for Airplane {
    fn fly(&self) -> FeaturePrimitiveArray<FeatureString> {
        println!("wjf airplane fly Called from C");
        let mut ret = FeaturePrimitiveArray::<FeatureString>::new(4);
        ret.append_string(FeatureString::from("airplane fly1"));
        ret.append_string(FeatureString::from("airplane fly2"));
        ret.append_string(FeatureString::from("airplane fly3"));
        ret.append_string(FeatureString::from("airplane fly4"));
        ret
    }

    fn get_breed(&self) -> FeatureString {
        println!("wjf airplane get_breed Called from C");
        FeatureString::new("airplane_get_breed")
    }

    fn set_breed(&self, _breed: &FeatureString) {
        println!("wjf airplane set_breed Called from C");
    }
}

impl Drop for Airplane {
    fn drop(&mut self) {
        println!("wjf Airplane droped");
    }
}

#[feature_instance(name = "Bird")]
pub struct Pigeon {}

// function implementation
impl Pigeon {
    fn new(handle: FeatureInstanceHandle) -> Self {
        Pigeon {
            instance: FeatureInstance::new(handle),
        }
    }
}

impl FeatureInstanceTrait for Pigeon {}

impl Animal for Pigeon {
    fn get_name(&self) -> FeatureString {
        println!("wjf pigeon get_name Called from C");
        FeatureString::new("pigeon_get_name")
    }

    fn set_name(&self, _name: &FeatureString) {
        println!("wjf pigeon set_name Called from C");
    }

    fn get_legCount(&self) -> FtInt {
        println!("wjf pigeon get_legCount Called from C");
        5
    }

    fn eatFood(&self, foods: &FeaturePrimitiveArray<FeatureString>) -> FtInt {
        println!("wjf pigeon eatFood Called from C");
        for i in 0..foods.len() {
            let item = foods.get(i).unwrap();
            println!("{} pigeon food: {}", i, *item);
        }
        foods.len() as i32
    }

    fn run(&self, _distance: FtInt, _destination: &FeatureString) -> FeatureString {
        println!("wjf pigeon run Called from C");
        FeatureString::new("pigeon_run")
    }
}

impl Flyable for Pigeon {
    fn fly(&self) -> FeaturePrimitiveArray<FeatureString> {
        println!("wjf pigeon fly Called from C");
        let mut ret = FeaturePrimitiveArray::<FeatureString>::new(3);
        ret.append_string(FeatureString::from("pigeon fly5"));
        ret.append_string(FeatureString::from("pigeon fly6"));
        ret.append_string(FeatureString::from("pigeon fly7"));
        ret
    }

    fn get_breed(&self) -> FeatureString {
        println!("wjf pigeon get_breed Called from C");
        FeatureString::new("pigeon_get_breed")
    }

    fn set_breed(&self, _breed: &FeatureString) {
        println!("wjf pigeon set_breed Called from C");
    }
}

impl Bird for Pigeon {
    fn get_weight(&self) -> FtInt {
        println!("wjf pigeon get_weight Called from C");
        7
    }

    fn set_weight(&self, _weight: FtInt) {
        println!("wjf pigeon set_weight Called from C");
    }

    fn walk(&self, _pid: FtPromiseId) {
        println!("wjf pigeon walk Called from C");
    }
}

impl Drop for Pigeon {
    fn drop(&mut self) {
        println!("wjf Pigeon droped");
    }
}

/*
#[feature_method_async("simple_wrap_foo")]
pub async fn foo(_feature: *mut c_void) -> *const i8 {
    println!("wjf foo Called from C");
    b"world\0".as_ptr() as *const i8
}

#[feature_method_async("simple_wrap_bar")]
pub async fn bar(_feature: *mut c_void, a: c_int, b: c_float) -> c_int {
    println!("wjf bar Called from C, a: {}, b: {}", a.to_string(), b);
    a + 1
}

#[feature_method_async("simple_wrap_goo")]
pub async fn goo(_feature: *mut c_void, a: c_double) -> c_double {
    println!("wjf goo Called from C, a: {}", a);
    a + 5.0
}

#[feature_method_async("simple_wrap_doo")]
pub async fn doo(_feature: *mut c_void) {
    println!("wjf doo Called from C")
}
*/
