// Style 1: C/Java style function declaration
int addNumbers(int a, int b) {
    return a + b;
}

// Style 2: SP-native function declaration
fn multiplyNumbers(int a, int b) -> int {
    return a * b;
}

// Style 3: C/Java style void function
void printMessage(string msg) {
    println("Message: " + msg);
}

// Inheritance Style 1: Java-style 'extends'
class Animal {
    string name;

    Animal(string n) {
        this.name = n;
    }

    fn speak() {
        println(this.name + " makes a sound");
    }
}

class Dog extends Animal {
    Dog(string n) {
        super(n);
    }

    override fn speak() {
        println(this.name + " barks: Woof!");
    }
}

// Inheritance Style 2: C#-style ':'
class Cat : Animal {
    Cat(string n) {
        super(n);
    }

    override fn speak() {
        println(this.name + " purrs: Meow!");
    }
}

fn main() {
    println("=== Multi-Syntax Compatibility Demo ===");

    // Functions
    println("addNumbers(10, 20):      " + addNumbers(10, 20));
    println("multiplyNumbers(5, 6):   " + multiplyNumbers(5, 6));
    printMessage("Multi-syntax compilation valid!");

    // OOP styles
    Dog dog = new Dog("Buddy");
    Cat cat = new Cat("Whiskers");

    dog.speak();
    cat.speak();

    // Switch vs Match compatibility
    int choice = 2;
    println("--- Switch statement ---");
    switch (choice) {
        case 1:
            println("Option 1");
            break;
        case 2:
            println("Option 2 selected");
            break;
        default:
            println("Default option");
    }

    println("--- Match statement ---");
    match choice {
        1 => println("Option 1"),
        2 => println("Option 2 matched"),
        _ => println("Default option")
    }
}
