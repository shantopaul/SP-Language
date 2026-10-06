// ═══════════════════════════════════════════════════
//  SINGLE-LINE COMMENT
// ═══════════════════════════════════════════════════

// This is a single-line comment
int appVersion = 1;  // This is an inline comment

// ═══════════════════════════════════════════════════
//  MULTI-LINE COMMENT
// ═══════════════════════════════════════════════════

/*
    This is a multi-line comment.
    It can span multiple lines.
    Useful for longer explanations and licenses.
*/

// ═══════════════════════════════════════════════════
//  DOCUMENTATION COMMENTS (processed by spc doc)
// ═══════════════════════════════════════════════════

/// Calculates the sum of two integers.
///
/// @param a The first integer.
/// @param b The second integer.
/// @return The sum of a and b.
fn add(int a, int b) -> int {
    return a + b;
}

/// Represents an authenticated user in the SP application.
class User {
    /// The user's full name.
    string name;

    /// The user's age in years.
    int age;

    User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    /// Displays the user information to stdout.
    fn display() {
        println("User: " + this.name + ", age " + this.age);
    }
}

fn main() {
    println("=== Comments & Documentation Showcase ===");

    int sum = add(10, 20);
    println("add(10, 20): " + sum);

    User u = new User("Shanto", 21);
    u.display();
}
