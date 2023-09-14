let interface = require('Interface');

function show_array(arr, pre) {
  interface.print(pre)
  for (var e in arr) {
    interface.print('[%s]=%s', e, arr[e])
  }
  interface.print('\n')
};

interface.print('\ntest begin')
let cat = interface.createCat()
interface.print('cat name: ', cat.name)
cat.name = "dotty"
interface.print('cat legCount: ', cat.legCount)
let cat_foods = ['fish', 'meat', 'beef', 'seefood']
let cat_eated = cat.eatFood(cat_foods)
interface.print('cat eated: ', cat_eated)
let cat_ran = cat.run(50, "home")
interface.print('cat ran: ', cat_ran)
let cat_fly = cat.fly()
show_array(cat_fly, 'cat fly:');
cat.walk().then(a => {
    interface.print("cat walk resolve: ");
    show_array(a, 'cat walk:');
}, b => {
    interface.print("cat walk reject: ", b);
})
interface.setAnimal(cat)
interface.print('\n')

let dog = interface.createDog(1)
interface.print('dog name: ', dog.name)
dog.name = "tom"
interface.print('dog legCount: ', dog.legCount)
let dog_foods = ['bone', 'soup', 'beef', 'ham', 'pizza', 'fish']
let dog_eated = dog.eatFood(dog_foods)
interface.print('dog eated: ', dog_eated)
let dog_ran = dog.run(200, "wild")
interface.print('dog ran: ', dog_ran)
let dog_fly = dog.fly()
show_array(dog_fly, 'dog fly:');
dog.walk().then(a => {
    interface.print("dog walk resolve: ");
    show_array(a, 'dog walk:');
}, b => {
    interface.print("dog walk reject: ", b);
})
interface.setAnimal(dog)

interface.flyFar(150).then(a => {
    interface.print("flyFar resolve: ");
    show_array(a, 'flyFar: ');
}, b => {
    interface.print("flyFar reject: ", b);
})
interface.flyAway().then(a => {
    interface.print("flyFar resolve: ");
    show_array(a, 'flyFar: ');
}, b => {
    interface.print("flyFar reject: ", b);
})
interface.print('\n')

interface.print('test ended\n')
