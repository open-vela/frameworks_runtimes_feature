import { interface_test, Animal, Bird, Chicken } from "./interface_test.d";

export function main() {
    // create feature module object
    let test = new interface_test();

    let dog: Animal = test.createDog(0);
    let cat: Animal = test.createCat();
    let pigeon: Bird = test.createPigeon();
    let cock: Chicken = test.createCock();
}
