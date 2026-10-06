class Animal {
    protected string name;
    protected int age;

    Animal(string name, int age) {
        this.name = name;
        this.age = age;
    }

    fn speak() {
        println(this.name + " makes a sound.");
    }

    fn info() {
        println(this.name + ", age " + this.age);
    }
}

class Dog extends Animal {
    string breed;

    Dog(string name, int age, string breed) {
        super(name, age);
        this.breed = breed;
    }

    override fn speak() {
        println(this.name + " says: Woof! 🐕");
    }

    fn fetch() {
        println(this.name + " fetches the ball! 🎾");
    }
}

class Cat extends Animal {
    bool isIndoor;

    Cat(string name, int age, bool isIndoor) {
        super(name, age);
        this.isIndoor = isIndoor;
    }

    override fn speak() {
        println(this.name + " says: Meow! 🐱");
    }

    fn purr() {
        println(this.name + " is purring... 😺");
    }
}

class Bird : Animal {
    Bird(string name, int age) {
        super(name, age);
    }

    override fn speak() {
        println(this.name + " says: Tweet! 🐦");
    }
}

fn main() {
    Dog dog = new Dog("Buddy", 3, "Golden Retriever");
    Cat cat = new Cat("Whiskers", 5, true);
    Bird bird = new Bird("Tweety", 2);

    println("=== Animal Shelter ===");
    println("");

    dog.info();
    dog.speak();
    println("");

    cat.info();
    cat.speak();
    println("");

    bird.info();
    bird.speak();
    println("");

    dog.fetch();
    cat.purr();
}
