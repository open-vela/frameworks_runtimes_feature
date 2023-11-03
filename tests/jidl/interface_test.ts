import { interface_test, Animal, Bird, Chicken } from "./interface_test.d";

function show_array(pre: string, arr: string[]): void {
    console.log(pre)
    for (var i = 0; i < arr.length; i++) {
        console.log(i, "return array element", arr[i])
    }
};

export function main() {
    // create feature module object
    let test = new interface_test();

    console.log('test begin')
    let cat: Animal = test.createCat();
    console.log("[ts] cat name is: ", cat.name)
    cat.name = "dotty";
    console.log('[ts] cat legCount: ', cat.legCount)
    const cat_foods = ['fish', 'meat', 'beef', 'seefood'];
    const cat_eated = cat.eatFood(cat_foods);
    console.log('cat eated: ', cat_eated)
    let cat_ran = cat.run(50, "home")
    console.log('cat ran: ', cat_ran)
    test.setAnimal(cat);

    let dog: Animal = test.createDog(1);
    console.log("[ts] dog name is: ", dog.name)
    dog.name = "tom";
    console.log('[ts] dog legCount: ', dog.legCount)
    let dog_foods = ['bone', 'soup', 'beef', 'ham', 'pizza', 'fish']
    let dog_eated = dog.eatFood(dog_foods);
    console.log('dog eated: ', dog_eated);
    let dog_ran = dog.run(200, "wild");
    console.log('dog ran: ', dog_ran);
    test.setAnimal(dog);

    let pigeon: Bird = test.createPigeon();
    pigeon.fly();
    console.log('pigeon breed: ', pigeon.breed)
    pigeon.breed = "King Pigeon";
    let pigeon_fly = pigeon.fly()
    show_array('pigeon fly:', pigeon_fly)

    let cock: Chicken = test.createCock();
    console.log('cock name: ', cock.name)
    cock.name = "gaga"
    console.log('cock legCount: ', cock.legCount)
    let cock_foods = ['wheat', 'grain', 'corn', 'earthworm', 'beetle']
    let cock_eated = cock.eatFood(cock_foods)
    console.log('cock eated: ', cock_eated)
    let cock_ran = cock.run(50, "garden")
    console.log('cock ran: ', cock_ran)
    console.log('cock breed: ', cock.breed)
    let cock_fly = cock.fly()
    show_array('cock fly:', cock_fly)
    console.log('cock weight: ', cock.weight)
    cock.weight = 5

    // cock.walk().then((a: any) => {
    //     console.log("cock walk resolve: ", a);
    //     // show_array('cock walk:', a);
    // }, (b: any) => {
    //     console.log("cock walk reject: ", b);
    // })
    // console.log('\n')

    test.flyFar(150).then((a: any) => {
        console.log("flyFar resolve: ", a);
        // show_array('flyFar: ', a);
    }, (b: any) => {
        console.log("flyFar reject: ", b);
    })

    test.flyAway().then((a: any) => {
        console.log("flyFar resolve: ", a);
        // show_array('flyFar: ', a);
    }, (b: any) => {
        console.log("flyFar reject: ", b);
    })
    console.log('test ended')
}