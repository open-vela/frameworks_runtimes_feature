import { ATest , Person} from "./ATest.d";
export function main(){
    //create feature module object
    let a = new ATest();
    
    //normal function call
    let c: string = a.test1("wasm", 1024);
    console.log("in ts (native pass string) is:", c);
    
    //callback
    a.test2(5,(x:number,y:number)=>{ console.log("callback to ts:", x+y)});
    a.test3("hello", (x: string) => { console.log("callback to ts:", x)});

    //promise
    console.log("before promise");
    a.test4(0).then((x: number) => {
        console.log("test3 resolve x: ", x);
    }, (y: number) => {
        console.log("test3 reject y: ", y);
    });
    console.log("after promise");

    //varying parameter
    a.print("hello", "wasm");

    // property set/get
    a.idx = 1024;
    console.log("after set idx is 1024, and get_idx is:", a.idx);  // 1024

    // array as parameter
    let array: number[] = [1, 3, 5, 7];
    a.test5(array);

    // string array as return value
    let arr: string[] = a.test6(110);
    console.log("test6 return array len is",arr.length);  // number
    console.log("test6 return array[0] ",arr[0]);         // hello
    console.log("test6 return array[1] ",arr[1]);         // world

    let p: Person = { name: "zhangsan", gender: "male", age: 23};
    a.test7(1024, p);

    let per: Person = a.test8(123);
    console.log("call test8 end")
    console.log("test8 return value Person per.name", per.name);
    console.log("test8 return value Person per.gender", per.gender);
    console.log("test8 return value Person per.age", per.age);
}
