class Counter {
    var count: int;

    Counter(int initial) {
        this.count = initial;
    }

    fn increment(int step) {
        this.count = this.count + step;
    }

    fn getCount() -> int {
        return this.count;
    }
}

class Animal {
    fn makeSound() {
        println("Animal sound: generic");
    }

    fn soundId() -> int {
        return 100;
    }
}

class Dog extends Animal {
    override fn makeSound() {
        println("Dog sound: Woof!");
    }

    override fn soundId() -> int {
        return 200;
    }
}

struct Vector2D {
    int x;
    int y;
}

enum Status {
    Inactive,
    Active = 10,
    Suspended = 20
}

fn main() {
    println("=== SP Language Phase 5: Object System Demo ===");

    // 1. Class instantiation & methods
    var counter: Counter = new Counter(40);
    counter.increment(2);
    print("Counter value: ");
    println(counter.getCount());

    // 2. Polymorphic virtual dispatch
    var pet: Animal = new Dog();
    pet.makeSound();
    print("Pet sound ID: ");
    println(pet.soundId());

    // 3. Struct value type on stack
    Vector2D vec;
    vec.x = 15;
    vec.y = 25;
    print("Vector sum (15 + 25): ");
    println(vec.x + vec.y);

    // 4. Enums
    var s: Status = Status.Active;
    if (s == Status.Active) {
        println("Status is Active (10)");
    }
}
