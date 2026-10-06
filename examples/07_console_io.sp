import std.io;

fn main() {
    println("Hello, World!");
    print("Enter name: ");

    string name = readLine();
    println("Hello, ${name}!");

    print("Enter age: ");
    int age = int.parse(readLine());
    println("You are ${age} years old.");

    print("Enter score: ");
    double score = double.parse(readLine());
    println("Your score is ${score}");
}
