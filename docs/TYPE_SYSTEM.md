# SP Type System Specification

This document details the type system rules, hierarchy, conversions, and checks of the **SP Programming Language**.

---

## 1. Type Categories

The SP type system is divided into four main categories:

1. **Primitive Value Types:** Fixed-size integer, floating point, boolean, character, and byte types.
2. **Compound / Aggregate Types:** Structs (value types) and Arrays (`T[]`).
3. **Reference / Managed Types:** Classes, Interfaces, Strings, and Collections (`List<T>`, `Map<K, V>`).
4. **Special Types:** `void`, Nullable types (`T?`), Result types (`Result<T, E>`), and Function pointer / Lambda types (`fn(T) -> U`).

---

## 2. Primitive Type Specifications

| Type | Bit Width | Signedness | Min Value | Max Value |
|---|---|---|---|---|
| `int8` | 8 | Signed | -128 | 127 |
| `uint8` | 8 | Unsigned | 0 | 255 |
| `int16` | 16 | Signed | -32,768 | 32,767 |
| `uint16` | 16 | Unsigned | 0 | 65,535 |
| `int32` / `int` | 32 | Signed | -2,147,483,648 | 2,147,483,647 |
| `uint32` / `uint` | 32 | Unsigned | 0 | 4,294,967,295 |
| `int64` | 64 | Signed | -9,223,372,036,854,775,808 | 9,223,372,036,854,775,807 |
| `uint64` | 64 | Unsigned | 0 | 18,446,744,073,709,551,615 |
| `float` | 32 | IEEE 754 | ~1.18e-38 | ~3.40e+38 |
| `double` | 64 | IEEE 754 | ~2.23e-308 | ~1.79e+308 |
| `bool` | 1 | Boolean | `false` | `true` |
| `char` | 32 | Unicode scalar | U+0000 | U+10FFFF |
| `byte` | 8 | Unsigned | 0 | 255 |

---

## 3. Type Inference & Conversions

### 3.1 Type Inference
- `var x = 42;` — Inferred as mutable `int`.
- `let s = "Hello";` — Inferred as `string`.
- `const pi = 3.14159;` — Inferred as immutable `double`.
- Type inference occurs during semantic analysis and never degrades to dynamic typing.

### 3.2 Implicit Widening Conversions
The compiler allows safe implicit widening without data loss:
- `int8` → `int16` → `int32` → `int64`
- `uint8` → `uint16` → `uint32` → `uint64`
- `float` → `double`
- `int32` → `double`

Narrowing conversions (e.g. `int64` to `int32` or `double` to `float`) require an explicit cast: `cast<int32>(val)` or `(int32)val`.

---

## 4. Null Safety System

- Non-nullable by default: A variable of type `T` cannot hold `null`.
  ```sp
  string s = null; // Compile error: E1008 Null Safety Violation
  ```
- Nullable types: Expressed with suffix `?`: `T?`.
  ```sp
  string? maybeStr = null; // Valid
  ```
- Safe Navigation: `expr?.member` evaluates to `null` if `expr` is null.
- Null Coalescing: `expr ?? fallback` returns `fallback` if `expr` is null.
