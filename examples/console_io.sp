import std.io;

fn main() {
    // ═══════════════════════════════════════
    //  OUTPUT
    // ═══════════════════════════════════════

    println("Hello, World!");           // Print with newline
    print("Enter name: ");             // Print without newline

    // ═══════════════════════════════════════
    //  INPUT
    // ═══════════════════════════════════════

    string name = readLine();
    println("Hello, ${name}!");

    print("Enter age: ");
    int age = int.parse(readLine());
    println("You are ${age} years old.");

    print("Enter score: ");
    double score = double.parse(readLine());
    println("Your score is ${score}");
}
