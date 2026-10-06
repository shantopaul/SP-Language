fn main() {
    int number = 42;
    uint positive = 100;

    int8 tiny = 127;
    int16 small = 32000;
    int32 medium = 2147483647;
    int64 large = 9223372036854;

    uint8 uTiny = 255;
    uint16 uSmall = 65535;
    uint32 uMedium = 4294967295;
    uint64 uLarge = 18446744073;

    float price = 29.99;
    double precise = 3.141592653589793;

    bool alive = true;
    bool dead = false;

    char letter = 'A';
    char symbol = '@';

    string text = "Hello, SP!";
    string empty = "";

    byte raw = 0xFF;

    println("int:    ${number}");
    println("float:  ${price}");
    println("double: ${precise}");
    println("bool:   ${alive}");
    println("char:   ${letter}");
    println("string: ${text}");
    println("byte:   ${raw}");
}
