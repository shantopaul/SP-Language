class Box<T> {
    var value: T;

    fn Box(val: T) {
        this.value = val;
    }

    fn getValue() -> T {
        return this.value;
    }

    fn setValue(newVal: T) {
        this.value = newVal;
    }
}

class Pair<A, B> {
    var first: A;
    var second: B;

    fn Pair(f: A, s: B) {
        this.first = f;
        this.second = s;
    }

    fn getFirst() -> A {
        return this.first;
    }

    fn getSecond() -> B {
        return this.second;
    }
}

fn identity<T>(item: T) -> T {
    return item;
}

fn main() {
    println("=== Generics Demo ===");

    var intBox = new Box<int>(42);
    println("Box<int> contains: " + intBox.getValue());
    intBox.setValue(100);
    println("Box<int> updated: " + intBox.getValue());

    var strBox = new Box<string>("Hello SP");
    println("Box<string> contains: " + strBox.getValue());

    var pair = new Pair<string, int>("Shanto", 21);
    println("Pair: (" + pair.getFirst() + ", " + pair.getSecond() + ")");

    var num = identity<int>(999);
    println("identity<int>: " + num);

    var text = identity<string>("SP Language");
    println("identity<string>: " + text);
}
