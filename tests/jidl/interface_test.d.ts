// Copyright 2023 Xiaomi, Inc. All rights reserved.

export class Animal {
  private instance: number;
  constructor() {
    this.init_native(this.clazz_name);
  }

  // parent member defines
  // self member defines
  get name(): string {
    return this.get_name_0();
  }

  set name(v: string) {
    this.set_name_0(v);
  }

  get legCount(): number {
    return this.get_legCount_0();
  }

  declare eatFood(foods: string[]): number;

  declare run(distance: number, destination: string): string;


  readonly clazz_name = "Animal";
  declare get_name_0(): string;
  declare set_name_0(v: string): void;
  declare get_legCount_0(): number;
  declare init_native(i_name: string): void;
}

export class Bird {
  private instance: number;
  constructor() {
    this.init_native(this.clazz_name);
  }

  // parent member defines
  // self member defines

  declare fly(): string[];

  get breed(): string {
    return this.get_breed_0();
  }

  set breed(v: string) {
    this.set_breed_0(v);
  }

  readonly clazz_name = "Bird";
  declare init_native(i_name: string): void;
  declare get_breed_0(): string;
  declare set_breed_0(v: string): void;
}

export class Chicken {
  private instance: number;
  constructor() {
    this.init_native(this.clazz_name);
  }

  // parent member defines
  get name(): string {
    return this.get_name_0();
  }
  set name(v: string) {
    this.set_name_0(v);
  }

  get legCount(): number {
    return this.get_legCount_0();
  }

  declare eatFood(foods: string[]): number;

  declare run(distance: number, destination: string): string;

  declare fly(): string[];

  get breed(): string {
    return this.get_breed_0();
  }

  set breed(v: string) {
    this.set_breed_0(v);
  }


  // self member defines
  get weight(): number {
    return this.get_weight_0();
  }

  set weight(v: number) {
    this.set_weight_0(v);
  }

  declare walk(): any;

  readonly clazz_name = "Chicken";
  declare init_native(i_name: string): void;
  declare get_name_0(): string;
  declare set_name_0(v: string): void;
  declare get_legCount_0(): number;
  declare get_breed_0(): string;
  declare set_breed_0(v: string): void;
  declare get_weight_0(): number;
  declare set_weight_0(v: number): void;
}


export class interface_test {
  constructor() {
    this.init_native(this.clazz_name);
  }
  /****** for JIDL Interface constructor function 'createDog' ******/
  createDog(type: number): Animal {
    let instance = this._createDog(type);
    let dog = new Animal();
    dog.instance = instance;
    return dog;
  }
  declare _createDog(type: number): number;

  /****** for JIDL Interface constructor function 'createPigeon' ******/
  createPigeon(): Bird {
    let instance = this._createPigeon();
    let pigeon = new Bird();
    pigeon.instance = instance;
    return pigeon;
  }
  declare _createPigeon(): number;

  /****** for JIDL Interface constructor function 'createCock' ******/
  createCock(): Chicken {
    let instance = this._createCock();
    let cock = new Chicken();
    cock.instance = instance;
    return cock;
  }
  declare _createCock(): number;

  createCat(): Animal {
    let instance = this._createCat();
    let cat = new Animal();
    cat.instance = instance;
    return cat;
  }
  declare _createCat(): number;

  declare setAnimal(animal: Animal): void;
  declare flyFar(distance: number): any;

  flyAway(): any {
    return this.flyFar(100);
  }
  declare print(...rest: any[]): void;

  // private:
  readonly clazz_name = "interface_test";
  declare init_native(name: string): void;
}