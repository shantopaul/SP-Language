fn main() {
    println("=== String Methods Showcase ===");

    string text = "  Hello, SP Programming Language!  ";
    string clean = text.trim();

    println("Original:   '" + text + "'");
    println("Trimmed:    '" + clean + "'");
    println("Length:     " + clean.length());
    println("Upper:      " + clean.upper());
    println("Lower:      " + clean.lower());
    println("Contains:   " + clean.contains("SP"));
    println("IndexOf:    " + clean.indexOf("SP"));
    println("Replace:    " + clean.replace("SP", "SP-Lang"));
    println("Substring:  " + clean.substring(0, 5));
    println("CharAt(0):  " + clean.charAt(0));

    string csv = "apple,banana,cherry,date";
    string[] fruits = csv.split(",");
    println("");
    println("Split CSV fruits:");
    for (fruit in fruits) {
        println("  - " + fruit);
    }
}
