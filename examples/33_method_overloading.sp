class Printer {
    fn printInt(int value) {
        println("Integer: " + value);
    }

    fn printDouble(double value) {
        println("Double: " + value);
    }

    fn printString(string value) {
        println("String: " + value);
    }

    fn printBool(bool value) {
        println("Boolean: " + value);
    }

    fn printLabeled(string label, int value) {
        println(label + ": " + value);
    }

    fn printLabeledWithUnit(string label, int value, string unit) {
        println(label + ": " + value + " " + unit);
    }
}

fn main() {
    println("=== Method Overloading Showcase ===");

    Printer p = new Printer();
    p.printInt(42);
    p.printDouble(3.14);
    p.printString("Hello");
    p.printBool(true);
    p.printLabeled("Age", 21);
    p.printLabeledWithUnit("Weight", 75, "kg");
}
