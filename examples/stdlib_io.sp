import std.io;

fn main() {
    println("=== SP Standard Library: std.io ===");

    // Basic printing
    print("Direct print without newline: ");
    println("Followed by println!");

    // Primitive overloads
    println(42);
    println(3.14159);
    println(true);

    // Standard error output
    eprintln("[STDERR] Diagnostic message from std.io");

    // Static IO class
    IO.println("Printed via IO.println");
    IO.print("Printed via IO.print\n");
    IO.eprintln("[STDERR] Error via IO.eprintln");
    IO.flush();

    println("=== std.io Completed Successfully ===");
}
