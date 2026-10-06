class Counter {
    int count;

    Counter(int initial) {
        this.count = initial;
    }

    fn next() -> int {
        this.count = this.count + 1;
        return this.count;
    }

    fn get() -> int {
        return this.count;
    }
}

class Greeter {
    string greeting;

    Greeter(string prefix) {
        this.greeting = prefix;
    }

    fn greet(string name) -> string {
        return this.greeting + ", " + name + "!";
    }
}

fn makeCounter(int start) -> Counter {
    return new Counter(start);
}

fn makeGreeter(string prefix) -> Greeter {
    return new Greeter(prefix);
}

fn main() {
    println("=== Closures & State Encapsulation ===");

    Counter counter = makeCounter(0);
    println("counter(): " + counter.next());
    println("counter(): " + counter.next());
    println("counter(): " + counter.next());

    Greeter hello = makeGreeter("Hello");
    Greeter hi = makeGreeter("Hi");

    println(hello.greet("Shanto"));
    println(hi.greet("Alice"));
}
