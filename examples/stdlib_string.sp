import std.string;

fn main() {
    println("=== SP Standard Library: std.string ===");

    string text = "  Hello, SP Language!  ";

    // 1. Length, trim, case
    println("Original: '${text}'");
    string trimmed = text.trim();
    println("Trimmed: '${trimmed}'");
    string upperText = trimmed.upper();
    println("Upper: '${upperText}'");
    string lowerText = trimmed.lower();
    println("Lower: '${lowerText}'");
    println("Length: ${text.length()}");

    // 2. Search & substrings
    string spStr = "SP";
    println("Contains 'SP': ${trimmed.contains(spStr)}");
    string helloStr = "Hello";
    println("Starts with 'Hello': ${trimmed.startsWith(helloStr)}");
    string bangStr = "!";
    println("Ends with '!': ${trimmed.endsWith(bangStr)}");
    println("Index of 'SP': ${trimmed.indexOf(spStr)}");
    string sub = trimmed.substring(0, 5);
    println("Substring(0, 5): '${sub}'");
    string replaced = trimmed.replace("SP Language", "World");
    println("Replaced: '${replaced}'");

    // 3. String split & iteration
    string csv = "apple,banana,cherry,orange";
    string[] items = csv.split(",");
    println("\nSplit items (${items.length()}):");
    for (string item in items) {
        println("  - ${item}");
    }

    // 4. Reverse & Repeat
    string spLang = "SP Lang";
    println("\nReversed 'SP Lang': '${spLang.reverse()}'");
    string na = "Na ";
    println("Repeat 'Na ': '${na.repeat(4)}'");

    // 5. String class static methods
    string s1 = "shanto";
    println("Via String.upper: '${String.upper(s1)}'");
    string s2 = "  paul  ";
    println("Via String.trim: '${String.trim(s2)}'");

    println("=== std.string Completed Successfully ===");
}
