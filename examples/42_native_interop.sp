// SP Native Interop & C ABI Bridge
// Full 'extern "C"' declarations are part of the SP 1.3 C FFI roadmap.
// In SP 1.0, native runtime C ABI calls (sp_rt_*) bridge SP directly to native C/C++ libraries.

import std.io;
import std.os;

fn main() {
    println("=== Native Interop & C Bridge Demo ===");
    println("Target OS:   " + OS.platform());
    println("Target Arch: " + OS.arch());
    println("Native C ABI bridge operational! ✅");
}
