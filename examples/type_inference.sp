// SP Programming Language - Type Inference
// From SP-Syntax-Guide.md Section 4: Type Inference

fn main() {
    // ═══════════════════════════════════════
    //  var — mutable, type inferred
    // ═══════════════════════════════════════

    var age = 21;                 // inferred as int
    var name = "Shanto";          // inferred as string
    var price = 99.99;            // inferred as double
    var active = true;            // inferred as bool

    age = 22;                     // ✅ Can reassign
    name = "Shanto Paul";         // ✅ Can reassign

    // ═══════════════════════════════════════
    //  let — mutable inferred (alternative)
    // ═══════════════════════════════════════

    let count = 100;
    let message = "Hello";

    // ═══════════════════════════════════════
    //  const — immutable, type inferred
    // ═══════════════════════════════════════

    const PI = 3.14159;           // inferred as double, immutable
    const APP = "SP Lang";        // inferred as string, immutable

    // ❌ PI = 3.14;              // error: cannot assign to constant

    println("Age: ${age}");
    println("Name: ${name}");
    println("PI: ${PI}");
    println("App: ${APP}");
}
