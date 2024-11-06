let interface = require('Interface');

function show_array(arr, pre) {
  console.log(pre)
  for (var e in arr) {
    console.log('[%s]=%s', e, arr[e])
  }
  console.log('\n')
};

console.log('\ntest begin')
let cat = interface.createCat()
cat.name = "dotty"
console.log('cat name: ', cat.name)
console.log('cat legCount: ', cat.legCount)
let cat_foods = ['fish', 'meat', 'beef', 'seefood']
let cat_eated = cat.eatFood(cat_foods)
console.log('cat eated: ', cat_eated)
let cat_ran = cat.run(50, "home")
console.log('cat ran: ', cat_ran)
interface.setAnimal(cat)
console.log('\n')

let dog = interface.createDog(1)
console.log('dog name: ', dog.name)
dog.name = "tom"
console.log('dog name2: ', dog.name)
console.log('dog legCount: ', dog.legCount)
let dog_foods = ['bone', 'soup', 'beef', 'ham', 'pizza', 'fish']
let dog_eated = dog.eatFood(dog_foods)
console.log('dog eated: ', dog_eated)
let dog_ran = dog.run(200, "wild")
console.log('dog ran: ', dog_ran)
interface.setAnimal(dog)
console.log('\n')

let pigeon = interface.createPigeon()
console.log('pigeon breed: ', pigeon.breed)
pigeon.breed = "King Pigeon"
let pigeon_fly = pigeon.fly()
show_array(pigeon_fly, 'pigeon fly:')
console.log('\n')

let cock = interface.createCock()
console.log('cock name: ', cock.name)
cock.name = "gaga"
console.log('cock legCount: ', cock.legCount)
let cock_foods = ['wheat', 'grain', 'corn', 'earthworm', 'beetle']
let cock_eated = cock.eatFood(cock_foods)
console.log('cock eated: ', cock_eated)
let cock_ran = cock.run(50, "garden")
console.log('cock ran: ', cock_ran)
console.log('cock breed: ', cock.breed)
cock.breed = "Malay"
let cock_fly = cock.fly()
show_array(cock_fly, 'cock fly:')
console.log('cock weight: ', cock.weight)
cock.weight = 5

cock.walk().then(a => {
    console.log("cock walk resolve: ");
    show_array(a, 'cock walk:');
}, b => {
    console.log("cock walk reject: ", b);
})
console.log('\n')

interface.flyFar(150).then(a => {
    console.log("flyFar resolve: ");
    show_array(a, 'flyFar: ');
}, b => {
    console.log("flyFar reject: ", b);
})
interface.flyAway().then(a => {
    console.log("flyFar resolve: ");
    show_array(a, 'flyFar: ');
}, b => {
    console.log("flyFar reject: ", b);
})

console.log('test ended\n')
