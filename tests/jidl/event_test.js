let test = require('event_test');

function on_data_changed_1(data) {
    console.log("on data changed 1, data:", data);
};

function on_data_changed_2(data) {
    console.log("on data changed 2, data:", data);
};

function on_data_changed_3(data) {
    console.log("on data changed 3, data:", data);
};

function on_data_changed_4(data) {
    console.log("on data changed 4, data:", data);
};

let data_changed_array_1 = [
   on_data_changed_2,
   on_data_changed_3
]

let data_changed_array_2 = [
   on_data_changed_1,
   on_data_changed_4
]

console.log("testing event data_changed = on_data_changed_1")
test.data_changed = on_data_changed_1
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed = data_changed_array_1")
test.data_changed = data_changed_array_1
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed on 'on_data_changed_4'")
test.on('data_changed', on_data_changed_4)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed off 'on_data_changed_4'")
test.off('data_changed', on_data_changed_4)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed on 'data_changed_array_2'")
test.on('data_changed', data_changed_array_2)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed off 'data_changed_array_2'")
test.off('data_changed', data_changed_array_2)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event array data_changed = data_changed_array_2")
test.data_changed = data_changed_array_2
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed = null")
test.data_changed = null
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed = data_changed_array_1")
test.data_changed = data_changed_array_1
test.invoke_event('data_changed')
console.log("testing event data_changed off 'null'")
test.off('data_changed', null)
test.invoke_event('data_changed')
console.log('\n\n')

function on_state_changed_1(state) {
    console.log("on state changed 1, state:", state);
};

function on_state_changed_2(state) {
    console.log("on state changed 2, state:", state);
};

function on_state_changed_3(state) {
    console.log("on state changed 3, state:", state);
};

function on_state_changed_4(state) {
    console.log("on state changed 4, state:", state);
};

let state_changed_array_1 = [
   on_state_changed_2,
   on_state_changed_3
]

let state_changed_array_2 = [
   on_state_changed_1,
   on_state_changed_4
]

console.log("testing event state_changed = on_state_changed_1")
test.state_changed = on_state_changed_1
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed = state_changed_array_1")
test.state_changed = state_changed_array_1
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed on 'on_state_changed_4'")
test.on('state_changed', on_state_changed_4)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed off 'on_state_changed_4'")
test.off('state_changed', on_state_changed_4)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed on 'state_changed_array_2'")
test.on('state_changed', state_changed_array_2)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed off 'state_changed_array_2'")
test.off('state_changed', state_changed_array_2)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed = state_changed_array_2")
test.state_changed = state_changed_array_2
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed = null")
test.state_changed = null
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event getter")
test.state_changed = state_changed_array_2
let state_changed_handlers = test.state_changed
console.log('state_changed_handlers:\n', state_changed_handlers)
console.log('\n\n')

console.log("testing invalid events")
test.invoke_event('invalid_event')
console.log('\n\n')


console.log("testing interface events")
function on_weight_changed_1(weight) {
    console.log("on weight changed 1, weight:", weight);
}

function on_weight_changed_2(weight) {
    console.log("on weight changed 2, weight:", weight);
}

let weight_changed_array = [
   on_weight_changed_1,
   on_weight_changed_2
]
let dog = test.createDog(1)

console.log("testing interface dog.weight_changed = on_weight_changed_1")
dog.weight_changed = on_weight_changed_1
test.invoke_event('weight_changed')
console.log('\n\n')

console.log("testing interface dog.weight_changed = weight_changed_array")
dog.weight_changed = weight_changed_array
test.invoke_event('weight_changed')
console.log('\n\n')

console.log("testing interface dog.weight_changed = null")
dog.weight_changed = null
test.invoke_event('weight_changed')
console.log('\n\n')

console.log("testing interface dog.weight_changed on 'on_weight_changed_1'")
dog.on('weight_changed', on_weight_changed_1)
test.invoke_event('weight_changed')
console.log('\n\n')

console.log("testing interface dog.weight_changed on 'weight_changed_array'")
dog.on('weight_changed', weight_changed_array)
test.invoke_event('weight_changed')
console.log('\n\n')

console.log("testing interface dog.weight_changed off 'weight_changed_array'")
dog.off('weight_changed', weight_changed_array)
test.invoke_event('weight_changed')
console.log('\n\n')


let cock = test.createCock()
console.log("testing interface cock.weight_changed = weight_changed_array")
cock.weight_changed = weight_changed_array
cock.invoke('weight_changed')
console.log('\n\n')

console.log("testing interface cock.weight_changed = on_weight_changed_1")
cock.weight_changed = on_weight_changed_1
cock.invoke('weight_changed')
console.log('\n\n')

console.log("testing interface cock.weight_changed off 'on_weight_changed_1'")
cock.off('weight_changed', on_weight_changed_1)
cock.invoke('weight_changed')
console.log('\n\n')

console.log("testing interface cock.weight_changed on 'weight_changed_array'")
cock.on('weight_changed', weight_changed_array)
cock.invoke('weight_changed')
console.log('\n\n')

console.log("testing interface cock.weight_changed off 'weight_changed_array'")
cock.off('weight_changed', weight_changed_array)
cock.invoke('weight_changed')
console.log('\n\n')


function on_ascended_1(height) {
    console.log("on ascended 1, height:", height);
};

function on_ascended_2(height) {
    console.log("on ascended 2, height:", height);
};

let ascended_array = [
   on_ascended_1,
   on_ascended_2
]

console.log("testing interface cock.ascended = ascended_array")
cock.ascended = ascended_array
cock.invoke('ascended')
console.log('\n\n')

console.log("testing interface cock.ascended = on_ascended_1")
cock.ascended = on_ascended_1
cock.invoke('ascended')
console.log('\n\n')

console.log("testing interface cock.ascended on 'ascended_array'")
cock.on('ascended', ascended_array)
cock.invoke('ascended')
console.log('\n\n')

console.log("testing interface cock.ascended off 'ascended_array'")
cock.off('ascended', ascended_array)
cock.invoke('ascended')
console.log('\n\n')


function on_egg_layed_1(count) {
    console.log("on egg_layed 1, count:", count);
};

function on_egg_layed_2(count) {
    console.log("on egg_layed 2, count:", count);
};

let egg_layed_array = [
   on_egg_layed_1,
   on_egg_layed_2
]

console.log("testing interface cock.egg_layed = egg_layed_array")
cock.egg_layed = egg_layed_array
cock.invoke('egg_layed')
console.log('\n\n')

console.log("testing interface cock.egg_layed = on_egg_layed_1")
cock.egg_layed = on_egg_layed_1
cock.invoke('egg_layed')
console.log('\n\n')

console.log("testing interface cock.egg_layed off 'on_egg_layed_1'")
cock.off('egg_layed', on_egg_layed_1)
cock.invoke('egg_layed')
console.log('\n\n')

console.log("testing interface cock.egg_layed on 'egg_layed_array'")
cock.on('egg_layed', egg_layed_array)
cock.invoke('egg_layed')
console.log('\n\n')

console.log("testing interface cock.egg_layed off 'null'")
cock.off('egg_layed', null)
cock.invoke('egg_layed')
console.log('\n\n')
