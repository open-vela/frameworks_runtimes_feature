import { Simple } from "./simple.d";

function show_args(pre: string, ...args: any[]): void {
    console.log(pre, 'args: ')
    for (var i = 0; i < args.length; i++) {
        console.log(args[i])
    }
    console.log('\n')
};

function show_array(pre: string, arr: any[]): void {
    console.log(pre)
    for (var e in arr) {
        console.log('[%s]=%s', e, arr[e])
    }
    console.log('\n')
};

export function main() {
    // create feature module object
    let test = new Simple();

    // normal function call
    test.bar();
    let ubar6_ret = test.ubar6(2.5);
    console.log("ubar6_ret:", ubar6_ret);

    test.goo(3, 12, (x: number, y: string, z: number) => {
        console.log('goo:x=', x, ', y=', y, ',z=', z, '\n');
    });
    test.goo3((x: number, y: string, z: number) => {
        console.log('goo3:x=', x, ', y=', y, ',z=', z, '\n')
    });

    test.print('hello world!\n')
    test.print('before set, test.name=', test.name, '\n');
    test.name = 'joker'
    test.print('after set, test.name=', test.name, '\n');
    test.print('test.version=', test.version, '\n');

    let bar6_ret = test.bar6(7, 3.5, false);
    console.log("bar6_ret:", bar6_ret);
    test.foo3(355, 59.39923, (x: number, y: string, z: number) => {
        console.log('foo3 call by async in worker: x=', x, ', y=', y, ', z=', z, '\n');
    });

    test.foo2(200, 3.33333, (x: number, y: string, z: number) => {
        console.log('foo2 call by async in current: x=', x, ', y=', y, ', z=', z, '\n');
    }, (a: number, b: string, ...rest: any[]) => {
        show_args('cb4 by async in current:', rest);
    });

    test.goo2(
        (a: number, b: string, ...rest: any[]) => { show_args('cb2', rest); },
        () => { console.log('cb3, no args\n') },
        (...rest: any[]) => { show_args('cb4', rest); }
    );

    const foo_ret:number = test.foo(5, 'hello', 2.5);
    test.print('foo_ret: ', foo_ret, '\n');

    // let arr1:number[] = [1, 3, 5, 7, 9, 11, 13];
    // const bar2_ret:number = test.bar2(arr1);
    // test.print('test.bar2 return:', bar2_ret, '\n');

    let arr1:number[] = [1, 3, 5, 7, 9, 11, 13];
    test.bar2(arr1);

    test.bar5(6, 100, 'world');
    show_array('test.bar3:', test.bar3());
}
