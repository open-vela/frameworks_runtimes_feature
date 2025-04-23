let test = require('interface_test');

function show_array(arr, pre) {
  console.log(pre)
  for (var e in arr) {
    console.log('[%s]=%s', e, arr[e])
  }
};

console.log('test begin')
let cat = test.createCat()
console.log('cat name: ', cat.name)
cat.name = "dotty"
console.log('cat name again: ', cat.name)
console.log('cat legCount: ', cat.legCount)
let cat_foods = ['fish', 'meat', 'beef', 'seefood']
let cat_eated = cat.eatFood(cat_foods)
console.log('cat eated: ', cat_eated)
let cat_ran = cat.run(50, "home")
console.log('cat ran: ', cat_ran)
test.setAnimal(cat)
console.log('\n')

let dog = test.createDog(1)
console.log('dog name: ', dog.name)
dog.name = "tom"
console.log('dog legCount: ', dog.legCount)
let dog_foods = ['bone', 'soup', 'beef', 'ham', 'pizza', 'fish']
let dog_eated = dog.eatFood(dog_foods)
console.log('dog eated: ', dog_eated)
let dog_ran = dog.run(200, "wild")
console.log('dog ran: ', dog_ran)
test.setAnimal(dog)
console.log('\n')

let pigeon = test.createPigeon()
console.log('pigeon breed: ', pigeon.breed)
pigeon.breed = "King Pigeon"
let pigeon_fly = pigeon.fly()
show_array(pigeon_fly, 'pigeon fly:')
console.log('\n')

let cock = test.createCock()
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

test.flyFar(150).then(a => {
    console.log("flyFar resolve: ");
    show_array(a, 'flyFar: ');
}, b => {
    console.log("flyFar reject: ", b);
})
test.flyAway().then(a => {
    console.log("flyAway resolve: ");
    show_array(a, 'flyAway: ');
}, b => {
    console.log("flyAway reject: ", b);
})

console.log('test ended\n')
