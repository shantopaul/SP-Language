// SP Programming Language - Strings & Interpolation
// From SP-Syntax-Guide.md Section 6: Strings & Interpolation

fn main() {
    string firstName = "Shanto";
    string lastName = "Paul";
    int age = 21;

    // ═══════════════════════════════════════
    //  CONCATENATION
    // ═══════════════════════════════════════

    string fullName = firstName + " " + lastName;
    println(fullName);   // Shanto Paul

    // ═══════════════════════════════════════
    //  STRING INTERPOLATION  ${expression}
    // ═══════════════════════════════════════

    println("Hello, ${firstName}!");
    println("Full Name: ${firstName} ${lastName}");
    println("Age next year: ${age + 1}");
    println("Name length: ${firstName.length()}");
    println("Uppercase: ${firstName.upper()}");

    // ═══════════════════════════════════════
    //  ESCAPE SEQUENCES
    // ═══════════════════════════════════════

    println("Line 1\nLine 2");           // newline
    println("Tab\there");                // tab
    println("She said \"Hello\"");       // quotes
    println("Backslash: \\");            // backslash
    println("Unicode: \u0041");          // A

    // ═══════════════════════════════════════
    //  MULTI-LINE (future)
    // ═══════════════════════════════════════

    string poem = "Roses are red,\n"
                + "Violets are blue,\n"
                + "SP is awesome,\n"
                + "And so are you.";

    println(poem);
}
