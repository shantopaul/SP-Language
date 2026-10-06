// SP Unsafe Mode & Low-Level Memory Management
// Unsafe blocks and direct pointer arithmetic are part of the SP 1.3 Systems roadmap.
// In SP 1.0, safe buffer allocation and systems programming are fully supported.

fn main() {
    println("=== Low-Level Systems & Memory Demo ===");

    // Low-level buffer allocation
    int[] buffer = [83, 80, 49, 46, 48];

    println("Raw buffer allocated: " + buffer.length() + " elements");
    println("Buffer header element 0: " + buffer[0]);
    println("Buffer header element 1: " + buffer[1]);
    println("Safe memory managed by SP runtime! ✅");
}
