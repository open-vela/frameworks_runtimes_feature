declare class Person {
    name: string;
    gender: string;
    age: number;
}

export class ATest {
    
    constructor(){
        this.init_native(this.clazz_name);
    }

    declare test1(a: string, b: number): string;

    declare test2(a:number,cb:(x:number,y:number)=>void):void;
    
    declare test3(c: string, cb: (x: string) => void): void;

    declare test4(a:number):any;

    declare test5(arr: number[]): void;

    declare test6(num: number):string[];
    
    declare test7(num:number, p:Person): void;
    
    declare test8(num:number): Person;

    declare print(...a:any[]):void;

    set idx(v: number) {
        this.set_idx_0(v);
    }
    get idx(): number {
        return this.get_idx_0();
    }
    
// private:
    readonly clazz_name = "ATest";
    declare init_native(name: string): void;
    declare get_idx_0(): number;
    declare set_idx_0(num: number): void;
}
