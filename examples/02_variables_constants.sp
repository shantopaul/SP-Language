fn main() {
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

    // Constants (Immutable)
    const int MAX_USERS = 1000;
    const double TAX_RATE = 0.15;
    const string APP_NAME = "MyApp";

    println("App: ${APP_NAME}");
    println("Max Users: ${MAX_USERS}");
    println("Tax Rate: ${TAX_RATE}");
}
