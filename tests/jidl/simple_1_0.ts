import { Simple } from "./simple_1_0.d";

function show_args(pre: string, args: any[]): void {
    console.log(pre, 'args: ')
    for (var i = 0; i < args.length; i++) {
        console.log('the index', i, 'is', args[i])
    }
    // console.log('\n')
};

function show_array(pre: string, arrs: string[]): void {
    console.log(pre)
    for (var i = 0; i < arrs.length; i++) {
        console.log('the index', i, 'is', arrs[i])
    }
    // console.log('\n')
};

export function main() {
    // create feature module object
    let test = new Simple();

    test.bar();
    let ubar6_ret = test.ubar6(2.5);
    test.print("ubar6_ret:", ubar6_ret);

    test.goo(3, 12, (x:number, y:string, z:number) => {
        test.print('goo:x=', x, ', y=', y, ',z=', z);
    });
    test.goo3((x:number, y:string, z:number) => {
      test.print('goo3:x=', x, ', y=', y, ',z=', z)
    });

    test.print('hello world!')
    test.print('before set, test.name=', test.name);
    test.name = 'joker'
    test.print('after set, test.name=', test.name);
    test.print('test.version=', test.version);

    test.print('test.x=', Simple.x);
    test.print('test.y=', Simple.y);
    test.print('test.z=', Simple.z);

    let bar6_ret = test.bar6(7, 3.5, false);

    test.foo3(
            355,
            59.39923,
            (x:number, y:string, z:number) => {
            test.print('foo3 call by async in worker: x=', x, ',y=', y, ',z=', z);}
        );

    test.foo2(
            200,
            3.33333,
            (x:number, y:string, z:number) => {
            test.print('foo2 call by async in current: x=', x, ',y=', y, ',z=', z);},
            (a:number, b:string, rest:any[]) => {
                show_args('cb2 by async in current:', rest);
            }
        );

    test.goo2(
            (a:number, b:string, rest:any[]) => {
                show_args('cb2', rest);
            },
            () => { test.print('cb3, no args') },
            (rest: any[]) => {
                show_args('cb4', rest);
            }
        );

    let foo_ret:number = test.foo(5, 'hello', 2.5);
    test.print('test.foo return:', foo_ret);

    // let arr1:number[] = [1, 3, 5, 7, 9, 11, 13];
    // let bar2_ret = test.bar2(arr1);
    // test.print('test.bar2 return:', bar2_ret, '\n');

    test.bar5(6, 100, 'world');
    let ret_arr:string[] =  test.bar3();
    show_array('test.bar3:', ret_arr);
}
