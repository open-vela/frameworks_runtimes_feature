let test = require('unit_interface_test');
init_feat_filter("interfaceTest.*");

feat_test("interfaceTest", "dogTest", () => {
    let dog = test.createDog(1);
    feat_expect_true(dog !== undefined && dog !== null, "dogTest case 1 error!");
    feat_expect_true(dog.name === "dog", "dogTest case 2 error!");
    feat_expect_true(dog.legCount === 4, "dogTest case 3 error!");
    dog.name = "Tom";
    feat_expect_true(dog.name === "Tom", "dogTest case 4 error!");
    // read only
    dog.legCount = 0;
    feat_expect_true(dog.legCount === 4, "dogTest case 5 error!");
    feat_expect_true(dog.eatFood(["food", "food"]), "dogTest case 6 error!");
    feat_expect_true(dog.run(100, "home"), "dogTest case 7 error!")
    // para error
    feat_expect_true(dog.eatFood() instanceof Error, "dogTest case 8 error!");
    feat_expect_true(dog.run() instanceof Error, "dogTest case 9 error!");
    feat_expect_true(dog.eatFood(100) instanceof Error, "dogTest case 9 error!");

    try {
        dog = test.createDog(-1);
        feat_expect_true(false, "dogTest case 10 error!");
    } catch (error) {
        feat_expect_true(true, "dogTest case 10 error!");
    }
})

// no ctor test
feat_test("interfaceTest", "catTest", () => {
    let cat = test.createCat();
    feat_expect_true(cat !== undefined && cat !== null, "catTest case 1 error!");
    feat_expect_true(cat.name === "cat", "catTest case 2 error!");
    feat_expect_true(cat.legCount === 4, "catTest case 3 error!");
    cat.name = "Tom";
    feat_expect_true(cat.name === "Tom", "catTest case 4 error!");
    // read only
    cat.legCount = 0;
    feat_expect_true(cat.legCount === 4, "catTest case 5 error!");
    feat_expect_true(cat.eatFood(["food", "food"]), "catTest case 6 error!");
    feat_expect_true(cat.run(100, "home"), "catTest case 7 error!");
    // para error
    feat_expect_true(cat.eatFood() instanceof Error, "catTest case 8 error!");
    feat_expect_true(cat.run() instanceof Error, "catTest case 9 error!");
    feat_expect_true(cat.eatFood(100) instanceof Error, "catTest case 9 error!");
})

feat_test("interfaceTest", "pigeonTest", () => {
    let pigeon = test.createPigeon();
    feat_expect_true(pigeon !== undefined && pigeon !== null, "pigeonTest case 1 error!");
    feat_expect_true(pigeon.breed ===  0, "pigeonTest case 2 error!");
    pigeon.breed = 2;
    feat_expect_true(pigeon.breed === 2, "pigeonTest case 3 error!");
    let data = pigeon.fly();
    feat_expect_true(arraysEqual(data, ['fly', 'away']), "pigeonTest case 4 error!");
})

feat_test("interfaceTest", "cockTest", () => {
    let cock = test.createCock();
    feat_expect_true(cock !== undefined && cock !== null, "cockTest case 1 error!");
    feat_expect_true(cock.breed ===  0, "cockTest case 2 error!");
    cock.breed = 1;
    feat_expect_true(cock.breed === 1, "cockTest case 3 error!");
    let data = cock.fly();
    feat_expect_true(arraysEqual(data, ['fly', 'away']), "cockTest case 4 error!");
    feat_expect_true(cock.weight === 0, "cockTest case 5 error!");
    cock.weight = 1;
    feat_expect_true(cock.weight === 1, "cockTest case 6 error!");
    cock.walk().then(a => {
        feat_expect_true(arraysEqual(a, ['walk', 'away']), "cockTest case 7 error!");
    }, b => {
        console.log("cock walk reject: ", b.code);
    });
    feat_expect_true(cock.legCount === 4, "cockTest case 8 error!");
    feat_expect_true(cock.name === "cock", "cockTest case 9 error!");
    cock.name = "Tom";
    feat_expect_true(cock.name === "Tom", "cockTest case 10 error!");
})