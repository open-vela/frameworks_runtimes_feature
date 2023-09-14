let interface = require('interface');

interface.print('hello world !');
{
    let dog = interface.createDog();
    dog.printName();

    let cat = interface.createCat();
    cat.printName();

    // receive interface as parameter
    cat.receiveInterface(dog);
    dog.receiveInterface(cat);

    // constant with different initializer, use getter for delay initialize
    interface.print("get name first~~~~~~~");
    interface.print("cat name is: ", cat.name);
    interface.print("dog name is: ", dog.name);

    // do not enter initializer agin
    interface.print("\nget name again~~~~~~~");
    interface.print("cat name is: ", cat.name);
    interface.print("dog name is: ", dog.name);
}
interface.print("\n\ndog and cat will be destroyed befor we uninit FeatureManager...\n");
let dog1 = interface.createDog();
let dog2 = interface.createDog();
dog1.printName();
dog2.printName();
interface.print("\n\ndog1 and dog2 will be destroyed after we uninit FeatureManager...\n");

