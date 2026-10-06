main() {
    // Explicit types
    int age = 21;
    string name = "Shanto";
    double salary = 75000.50;
    float score = 95.5;
    bool isActive = true;
    char grade = 'A';

    println("Name: ${name}");
    println("Age: ${age}");
    println("Salary: ${salary}");
    println("Score: ${score}");
    println("Active: ${isActive}");
    println("Grade: ${grade}");

    // Variables can be reassigned
    age = 22;
    name = "Shanto Paul";
    println("Updated: ${name}, age ${age}");
}
