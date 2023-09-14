let interface = require('Interface');

interface.print('\ntest begin')
let cat = interface.createCat()
interface.print('cat name: ', cat.name)
cat.name = "dotty"
interface.print('cat legCount: ', cat.legCount)
cat.eatFood()
cat.run()
cat.fly()
interface.setAnimal(cat)
interface.print('\n')

let dog = interface.createDog(1)
interface.print('dog name: ', dog.name)
dog.name = "tom"
interface.print('dog legCount: ', dog.legCount)
dog.eatFood()
dog.run()
dog.fly()
interface.setAnimal(dog)
interface.print('test ended\n')
