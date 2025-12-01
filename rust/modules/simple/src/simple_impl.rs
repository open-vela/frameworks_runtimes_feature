use crate::simple::*;
use alloc::{boxed::Box, string::String, vec};
use async_trait::async_trait;
use core::time::Duration;
use feature_frm::*;
use feature_macros::feature_instance;
use vdk::async_runtime::time;
use vdk::log::{error, info};

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
    chapter: Option<Chapter>,
    book: Option<Book>,
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

    fn hoo(&mut self, a: &Option<FeatureString>) {
        let a = if let Some(a) = a {
            a
        } else {
            error!("simple hoo: a is none");
            return;
        };
        info!("wjf hoo Called from C, a: \"{}\"", a.as_str());
        if let Some(proto) = self.get_prototype() {
            info!("proto.str: {}", proto.str);
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
    }

    fn set_book(&mut self, book: Option<Book>) {
        let mut book = if let Some(book) = book {
            book
        } else {
            error!("simple set_book: book is none");
            return;
        };
        if let Some(book_name) = book.get_book_name() {
            info!("set_book called from C, book_name: {}", book_name);
        }

        // get and set book_info
        if let Some(book_info) = book.get_book_info() {
            info!("wjf book_info: {}", book_info.as_str());
        }
        let new_book_info = FeatureJsonObject::new("{\"author\": \"caoxueqin\", \"dynasty\": \"Tang dynasty\", \"class\": \"political novel\"}");
        book.set_book_info(new_book_info);

        // get and set chaps_1
        if let Some(chap1) = book.get_chap_1() {
            let chap1_title = chap1.get_title().unwrap_or("".into());
            info!(
                "wjf chap1 title: {}, page_count: {}, is_end: {}",
                chap1_title,
                chap1.get_page_count(),
                chap1.get_is_end()
            );
        }
        let mut new_chap1 = Chapter::new();
        new_chap1.set_title(FeatureString::new("chap ten"));
        new_chap1.set_page_count(150);
        new_chap1.set_is_end(true);
        book.set_chap_1(new_chap1);

        // get and set chaps_info
        if let Some(chaps_info) = book.get_chaps_info() {
            for i in 0..chaps_info.len() {
                if let Some(chap_info) = chaps_info.get(i) {
                    info!("wjf chap_info[{}]: {}", i, chap_info.as_str());
                }
            }
        }
        let mut new_chaps_info = FeatureReferenceArray::<FeatureJsonObject>::new(2);
        let chap1_info =
            FeatureJsonObject::new("{\"chap1\": {\"title\": \"chap one\", \"page_count\": 50}}");
        let chap2_info =
            FeatureJsonObject::new("{\"chap2\": {\"title\": \"chap two\", \"page_count\": 80}}");
        new_chaps_info.append(chap1_info);
        new_chaps_info.append(chap2_info);
        book.set_chaps_info(new_chaps_info);

        // take callback for future invoke
        self.chap_changed = book.take_chap_changed();
        self.book = Some(book);
    }

    fn get_book(&mut self) -> Option<Book> {
        info!("wjf get_book Called from C");
        self.book.clone()
    }

    fn set_chapter(&mut self, chap: Option<Chapter>) {
        let chap = if let Some(chap) = chap {
            chap
        } else {
            error!("simple set_chapter: chap is none");
            return;
        };
        info!(
            "set_chapter called from C, page_count:{}",
            chap.get_page_count()
        );
        if let Some(cb) = &self.chap_changed {
            if let Some(title) = chap.get_title() {
                // invoke callback
                cb.invoke(1, title);
            }
        } else {
            info!("No chap_changed callback available");
        }
        self.chapter = Some(chap);
    }

    fn get_chapter(&self) -> Option<Chapter> {
        self.chapter.clone()
    }

    fn set_chapter_array(&mut self, chap_array: Option<FeatureReferenceArray<Chapter>>) {
        let chap_array = if let Some(chap_array) = chap_array {
            chap_array
        } else {
            error!("simple set_chapter_array: chap_array is none");
            return;
        };
        info!("wjf set_chapter_array Called from C");
        for i in 0..chap_array.len() {
            if let Some(item) = chap_array.get(i) {
                info!(
                    "wjf i: {}, chap.page_count: {}, chap.title: {}, chap.is_end: {}",
                    i,
                    item.get_page_count(),
                    item.get_title().unwrap_or("".into()),
                    item.get_is_end()
                );
            };
        }
    }

    fn get_chapter_array(&mut self) -> Option<FeatureReferenceArray<Chapter>> {
        info!("wjf get_chapter_array Called from C");
        let mut ret = FeatureReferenceArray::<Chapter>::new(2);
        let mut chap1 = Chapter::new();
        chap1.set_title(FeatureString::new("chapter 1"));
        chap1.set_is_end(false);
        chap1.set_page_count(100);
        let mut chap2 = Chapter::new();
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

    fn create_dog(&self) -> Option<FeatureInstance> {
        info!("wjf create_dog Called from C");
        let instance = create_dog_instance(&self.instance);
        let dog = Dog::new(instance.clone());
        instance.attach(Box::new(dog) as Box<dyn Animal>);
        Some(instance)
    }

    fn create_airplane(&self) -> Option<FeatureInstance> {
        info!("wjf create_airplane Called from C");
        let instance = create_airplane_instance(&self.instance);
        instance.attach(Box::new(Airplane::new(instance.clone())) as Box<dyn Flyable>);
        Some(instance)
    }

    fn create_pigeon(&self) -> Option<FeatureInstance> {
        info!("wjf create_pigeon Called from C");
        let instance = create_pigeon_instance(&self.instance);
        let bird: Box<dyn Bird> = Box::new(Pigeon::new(instance.clone()));
        instance.attach(bird);
        Some(instance)
    }

    fn create_cat(&self) -> Option<FeatureInstance> {
        info!("wjf create_cat Called from C");
        None
    }

    fn set_animal(&self, _animal: FeatureInstance) {
        info!("wjf set_animal Called from C");
    }

    fn invoke_event(&self, name: &Option<FeatureString>) {
        let name = if let Some(name) = name.as_ref() {
            name
        } else {
            error!("simple invoke_event: name is none");
            return;
        };
        info!("wjf invoke_event Called from C, event_name: {}", name);
        if let Some(eid) = self.get_event_id(name.as_str()) {
            let cb_count = self.get_event_callback_count(eid);
            info!("wjf event_id: {}, cb_count: {}", eid, cb_count);
            if name.as_str() == "data_changed" {
                let event = DataChangedEvent::new(eid, self.instance.clone());
                event.emit(FeatureString::new("hello world"));
            } else if name.as_str() == "state_changed" {
                let event = StateChangedEvent::new(eid, self.instance.clone());
                event.emit(50);
            }
        }
    }

    fn set_buffer(&self, buffer: Option<FeatureArrayBuffer>) {
        let buffer = if let Some(buffer) = buffer {
            buffer
        } else {
            error!("simple set_buffer: buffer is none");
            return;
        };
        info!("wjf set_buffer Called from C");
        if let Some(vec) = buffer.as_slice::<u8>() {
            info!("wjf u8 arraybuffer length: {}, buff: {:#?}", vec.len(), vec);
        }
    }

    fn get_buffer_copy(&self) -> FeatureArrayBuffer {
        info!("wjf get_buffer_copy Called from C");
        let vec = vec![-1i8, 2, 3, 4, 5];

        FeatureArrayBuffer::from_slice_copy(&self.instance, &vec)
    }

    fn get_buffer_no_copy(&self) -> FeatureArrayBuffer {
        info!("wjf get_buffer_no_copy Called from C");

        FeatureArrayBuffer::from_vec(&self.instance, vec![-5i32, 4, 3, 2, 1])
    }

    fn set_buffer_array(&self, ab_array: Option<FeatureReferenceArray<FeatureArrayBuffer>>) {
        let ab_array = if let Some(ab_array) = ab_array {
            ab_array
        } else {
            error!("simple set_buffer_array: ab_array is none");
            return;
        };
        info!("wjf set_buffer_array Called from C");
        for i in 0..ab_array.len() {
            if let Some(ab) = ab_array.get(i) {
                if let Some(vec) = ab.as_slice::<u8>() {
                    info!("ab_array[{}]: {:#?}", i, vec);
                }
            }
        }
    }

    fn get_buffer_array_copy(&self) -> Option<FeatureReferenceArray<FeatureArrayBuffer>> {
        info!("wjf get_buffer_array_copy Called from C");
        let mut ret = FeatureReferenceArray::<FeatureArrayBuffer>::new(3);
        let vec1 = &[-1i32, -2, 3, 4, 5];
        let ab1 = FeatureArrayBuffer::from_slice_copy(&self.instance, vec1);
        let vec2 = &[5i32, 4, 3, 2, 1];
        let ab2 = FeatureArrayBuffer::from_slice_copy(&self.instance, vec2);
        let vec3 = &[6i32, 7, 8];
        let ab3 = FeatureArrayBuffer::from_slice_copy(&self.instance, vec3);
        ret.append(ab1);
        ret.append(ab2);
        ret.append(ab3);
        Some(ret)
    }

    fn get_buffer_array_no_copy(&self) -> Option<FeatureReferenceArray<FeatureArrayBuffer>> {
        info!("wjf get_buffer_array_no_copy Called from C");
        let mut ret = FeatureReferenceArray::<FeatureArrayBuffer>::new(2);
        let ab1 = FeatureArrayBuffer::from_vec(&self.instance, vec![-6i8, 7, 8, 9, 10]);
        let ab2 = FeatureArrayBuffer::from_vec(&self.instance, vec![10, 9, 8, 7, 6]);
        ret.append(ab1);
        ret.append(ab2);
        Some(ret)
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
    name: FeatureString,
    leg_count: FtInt,
}

// function implementation
impl Dog {
    fn new(instance: FeatureInstance) -> Self {
        Dog {
            instance,
            name: FeatureString::new("Puppy"),
            leg_count: 4,
        }
    }
}

impl Animal for Dog {
    fn get_name(&self) -> FeatureString {
        info!("wjf dog get_name Called from C");
        self.name.clone()
    }

    fn set_name(&mut self, name: Option<FeatureString>) {
        let name = if let Some(name) = name {
            name
        } else {
            error!("simple Dog set_name: name is none");
            return;
        };
        info!("wjf dog set_name Called from C, name: {}", name);
        self.name = name;
    }

    fn get_leg_count(&self) -> FtInt {
        info!("wjf dog get_leg_count Called from C");
        self.leg_count
    }

    fn eat_foods(&self, foods: &Option<FeaturePrimitiveArray<FeatureString>>) -> FtInt {
        let foods = if let Some(foods) = foods.as_ref() {
            foods
        } else {
            error!("simple Dog eat_food: foods is none");
            return -1;
        };
        info!("wjf dog eat_food Called from C");
        for i in 0..foods.len() {
            if let Some(item) = foods.get(i) {
                info!("{} dog food: {}", i, *item);
            }
        }
        foods.len() as i32
    }

    fn run(&self, _distance: FtInt, _destination: &Option<FeatureString>) -> FeatureString {
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
    breed: FeatureString,
}

// function implementation
impl Airplane {
    fn new(instance: FeatureInstance) -> Self {
        Airplane {
            instance,
            breed: FeatureString::new("Airbus"),
        }
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
        self.breed.clone()
    }

    fn set_breed(&mut self, breed: Option<FeatureString>) {
        let breed = if let Some(breed) = breed {
            breed
        } else {
            error!("simple Airplane set_breed: breed is none");
            return;
        };
        info!("wjf airplane set_breed Called from C, breed: {}", breed);
        self.breed = breed;
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
    name: FeatureString,
    breed: FeatureString,
    leg_count: FtInt,
    weight: FtInt,
}

// function implementation
impl Pigeon {
    fn new(instance: FeatureInstance) -> Self {
        Pigeon {
            instance,
            name: FeatureString::new("Googoo"),
            breed: FeatureString::new("Chinese pigeon"),
            leg_count: 2,
            weight: 10,
        }
    }
}

impl Animal for Pigeon {
    fn get_name(&self) -> FeatureString {
        info!("wjf pigeon get_name Called from C");
        self.name.clone()
    }

    fn set_name(&mut self, name: Option<FeatureString>) {
        let name = if let Some(name) = name {
            name
        } else {
            error!("simple Pigeon: name is none");
            return;
        };
        info!("wjf pigeon set_name Called from C, _name: {}", name);
        self.name = name;
    }

    fn get_leg_count(&self) -> FtInt {
        info!("wjf pigeon get_leg_count Called from C");
        self.leg_count
    }

    fn eat_foods(&self, foods: &Option<FeaturePrimitiveArray<FeatureString>>) -> FtInt {
        let foods = if let Some(foods) = foods.as_ref() {
            foods
        } else {
            error!("simple Pigeon eat_food: foods is none");
            return -1;
        };
        info!("wjf pigeon eat_food Called from C");
        for i in 0..foods.len() {
            if let Some(item) = foods.get(i) {
                info!("{} pigeon food: {}", i, *item);
            }
        }
        foods.len() as i32
    }

    fn run(&self, _distance: FtInt, _destination: &Option<FeatureString>) -> FeatureString {
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
        self.breed.clone()
    }

    fn set_breed(&mut self, breed: Option<FeatureString>) {
        let breed = if let Some(breed) = breed {
            breed
        } else {
            error!("simple Pigeon set_breed: breed is none");
            return;
        };
        info!("wjf pigeon set_breed Called from C, breed: {}", breed);
        self.breed = breed;
    }
}

impl Bird for Pigeon {
    fn get_weight(&self) -> FtInt {
        info!("wjf pigeon get_weight Called from C");
        self.weight
    }

    fn set_weight(&mut self, weight: FtInt) {
        info!("wjf pigeon set_weight Called from C, weight: {}", weight);
        self.weight = weight;
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
