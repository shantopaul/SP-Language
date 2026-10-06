// SP Programming Language - All Primitive Types
// From SP-Syntax-Guide.md Section 3: Data Types

fn main() {
    // ═══════════════════════════════════════
    //  INTEGER TYPES
    // ═══════════════════════════════════════

    int number = 42;              // Platform default (32 or 64 bit)
    uint positive = 100;          // Unsigned integer

    int8 tiny = 127;              // -128 to 127
    int16 small = 32000;          // -32768 to 32767
    int32 medium = 2147483647;    // Standard 32-bit
    int64 large = 9223372036854;  // 64-bit

    uint8 uTiny = 255;            // 0 to 255
    uint16 uSmall = 65535;        // 0 to 65535
    uint32 uMedium = 4294967295;  // 0 to 4,294,967,295
    uint64 uLarge = 18446744073;  // 0 to 18,446,744,073,709,551,615

    // ═══════════════════════════════════════
    //  FLOATING POINT TYPES
    // ═══════════════════════════════════════

    float price = 29.99;          // 32-bit floating point
    double precise = 3.141592653589793;  // 64-bit floating point

    // ═══════════════════════════════════════
    //  OTHER TYPES
    // ═══════════════════════════════════════

    bool alive = true;            // true or false
    bool dead = false;

    char letter = 'A';            // Single character
    char symbol = '@';
    char newline = '\n';

    string text = "Hello, SP!";   // String of characters
    string empty = "";

    byte raw = 0xFF;              // Raw byte (0-255)

    // Print all
    println("int:    ${number}");
    println("float:  ${price}");
    println("double: ${precise}");
    println("bool:   ${alive}");
    println("char:   ${letter}");
    println("string: ${text}");
    println("byte:   ${raw}");
}
