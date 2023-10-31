import { interface_test, Animal, Bird, Chicken } from "./interface_test.d";

export function main() {
    // create feature module object
    let test = new interface_test();
    const a = ["hello", "dog"];
    let dog: Animal = test.createDog(0);
    dog.eatFood(a);

    const b = ["hello", "cat"];
    let cat: Animal = test.createCat();
    cat.eatFood(b);

    let pigeon: Bird = test.createPigeon();
    pigeon.fly();

    let cock: Chicken = test.createCock();
    cock.run(42, "wuhan");
}