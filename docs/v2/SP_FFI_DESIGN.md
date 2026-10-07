# SP Language — Foreign Function Interface (FFI) & C ABI Design

**Document ID:** SP-FFI-001  
**Status:** Architecture Blueprint  

---

## 1. Vision for SP FFI

SP's goal is to provide **zero-overhead, frictionless interoperability with native C libraries**. A developer should be able to call SQLite, OpenSSL, raylib, or libc functions with zero glue code and zero marshaling overhead.

---

## 2. Current FFI Implementation State

SP already possesses a working `extern "C"` declaration mechanism used internally by `stdlib/*.sp`:
```sp
extern "C" {
    fn puts(string s) -> int;
    fn malloc(int size) -> ptr;
    fn free(ptr p);
}
```
The compiler currently emits native C-calling-convention declarations directly into LLVM IR (`declare i32 @puts(ptr)`).

---

## 3. Required Enhancements for Production FFI

### 3.1 Raw Pointers (`ptr`, `ptr<T>`)
- Distinguish between managed SP references (`class`) and unmanaged raw memory pointers (`ptr<T>`).
- Direct pointer dereferencing and arithmetic within explicit `unsafe` blocks.

### 3.2 C Struct Memory Layout (`#[repr(C)]`)
- Guarantee byte-for-byte alignment matching the target platform’s C ABI:
  ```sp
  #[repr(C)]
  struct Point {
      int x;
      int y;
  }
  ```

### 3.3 Dynamic Library Loading
- Support linking both static libraries (`.a`, `.lib`) and dynamic libraries (`.so`, `.dll`, `.dylib`) via `sp.toml`:
  ```toml
  [package.native]
  links = ["sqlite3", "ssl"]
  ```

### 3.4 C Header Binding Generator (`spc bindgen`)
- A built-in CLI tool to parse `.h` header files via libclang and automatically emit SP `extern "C"` declarations.
