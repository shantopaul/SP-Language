// SP Programming Language - Constants (Immutable)
// From SP-Syntax-Guide.md Section 2: Variables & Constants

fn main() {
    const int MAX_USERS = 1000;
    const double TAX_RATE = 0.15;
    const string APP_NAME = "MyApp";

    println("App: ${APP_NAME}");
    println("Max Users: ${MAX_USERS}");
    println("Tax Rate: ${TAX_RATE}");

    // ❌ This will NOT compile:
    // MAX_USERS = 2000;    // error: cannot assign to constant
}
