let interface = require('interface_test');

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
interface.setAnimal(dog)
interface.print('\n')

let pigeon = interface.createPigeon()
interface.print('pigeon breed: ', pigeon.breed)
pigeon.breed = "King Pigeon"
let pigeon_fly = pigeon.fly()
show_array(pigeon_fly, 'pigeon fly:')
interface.print('\n')

let cock = interface.createCock()
interface.print('cock name: ', cock.name)
cock.name = "gaga"
interface.print('cock legCount: ', cock.legCount)
let cock_foods = ['wheat', 'grain', 'corn', 'earthworm', 'beetle']
let cock_eated = cock.eatFood(cock_foods)
interface.print('cock eated: ', cock_eated)
let cock_ran = cock.run(50, "garden")
interface.print('cock ran: ', cock_ran)
interface.print('cock breed: ', cock.breed)
cock.breed = "Malay"
let cock_fly = cock.fly()
show_array(cock_fly, 'cock fly:')
interface.print('cock weight: ', cock.weight)
cock.weight = 5

cock.walk().then(a => {
    interface.print("cock walk resolve: ");
    show_array(a, 'cock walk:');
}, b => {
    interface.print("cock walk reject: ", b);
})
interface.print('\n')

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

interface.print('test ended\n')
