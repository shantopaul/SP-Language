# SP Programming Language

## Complete Language Specification, Compiler Architecture, Syntax Design & Development Roadmap

**Project Name:** SP Programming Language  
**Short Name:** SP  
**Source Extension:** `.sp`  
**Compiler:** `spc`  
**Package Manager:** `spm`  
**Language Website:** `splang.shantopaul.com` *(future)*  
**Repository:** `github.com/shantopaul/sp-language` *(proposed)*  
**Language Type:** General-Purpose Programming Language  
**Primary Paradigms:** Procedural, Object-Oriented, Functional, Generic  
**Target Platforms:** Windows, Linux, macOS  
**Initial Implementation Language:** C++ or Rust  
**Initial Compiler Strategy:** LLVM-based native compiler  
**Alternative Runtime:** SP Virtual Machine  

---

# 1. Vision

SP is a modern general-purpose programming language designed to combine the familiar syntax of:

- C
- C++
- Java
- C#
- JavaScript/TypeScript-inspired conveniences

while providing its own:

- type system
- standard library
- module system
- compiler
- runtime
- package manager
- tooling
- formatter
- debugger
- testing framework

The goal is that a developer who already knows Java, C#, or C++ can begin writing SP code without learning an entirely unfamiliar syntax.

SP should remain its own programming language rather than becoming a Java/C#/C++ transpiler.

---

# 2. Core Design Philosophy

SP follows these principles:

## 2.1 Familiar Syntax

Developers should recognize common constructs immediately.

Example:

```sp
class User {
    private string name;

    public User(string name) {
        this.name = name;
    }

    public string getName() {
        return this.name;
    }
}
```

This should feel familiar to Java/C# developers.

---

## 2.2 Modern Type Safety

SP should be statically typed.

Example:

```sp
int age = 20;
string name = "Shanto";
bool active = true;
```

Invalid:

```sp
int age = "hello";
```

The compiler should report a compile-time error.

---

## 2.3 Automatic Memory Management

The initial SP implementation should use automatic memory management.

The recommended model is:

- stack allocation where possible
- ownership/RAII where appropriate
- garbage collection for managed objects, OR
- LLVM-based ownership/reference-counting runtime

The first stable implementation should prioritize developer usability.

---

## 2.4 Performance

SP should be capable of producing native executable binaries.

Example:

```text
hello.sp
    ↓
SP Compiler
    ↓
SP AST
    ↓
Typed AST
    ↓
LLVM IR
    ↓
Machine Code
    ↓
hello.exe
```

---

# 3. Language Identity

SP should have its own identity.

It should NOT be:

```text
Java with a different extension
```

or:

```text
C# transpiler
```

or:

```text
C++ wrapper
```

Instead:

```text
SP
├── SP Syntax
├── SP Type System
├── SP Compiler
├── SP Runtime
├── SP Standard Library
├── SP Package Manager
└── SP Toolchain
```

---

# 4. File Extension

SP source files use:

```text
.sp
```

Examples:

```text
main.sp
hello.sp
user.sp
database.sp
server.sp
math.sp
```

A complete project could look like:

```text
my-project/
│
├── sp.toml
├── src/
│   ├── main.sp
│   ├── user.sp
│   └── database.sp
│
├── tests/
│   └── user_test.sp
│
├── assets/
│
└── README.md
```

---

# 5. Hello World

The simplest SP program:

```sp
import std.io;

fn main() {
    println("Hello, World!");
}
```

Alternative C/Java-style form:

```sp
public static void main() {
    System.out.println("Hello, World!");
}
```

However, the recommended native SP syntax is:

```sp
fn main() {
    println("Hello, World!");
}
```

---

# 6. Multi-Syntax Compatibility

SP should support multiple familiar syntax styles.

The goal is NOT to accept every possible Java/C#/C++ grammar.

Instead, SP should support a controlled compatibility layer.

For example:

```sp
int age = 20;
```

This syntax is familiar across C, C++, Java and C#.

SP should accept it.

---

# 7. Variables

Standard SP syntax:

```sp
int age = 20;
string name = "Shanto";
bool active = true;
double price = 99.99;
```

Immutable variable:

```sp
const int maxUsers = 100;
```

Type inference:

```sp
var age = 20;
var name = "Shanto";
```

Immutable inferred value:

```sp
const name = "Shanto";
```

---

# 8. Primitive Types

SP should initially support:

```text
int
uint
int8
int16
int32
int64

uint8
uint16
uint32
uint64

float
double

bool
char

string

byte
```

Example:

```sp
int age = 21;
double salary = 50000.50;
float percentage = 95.5;
bool isStudent = true;
char grade = 'A';
string name = "Shanto";
```

---

# 9. Void

Functions that return nothing:

```sp
void printName() {
    println("Shanto");
}
```

Recommended modern syntax:

```sp
fn printName() {
    println("Shanto");
}
```

Both can be supported.

---

# 10. Functions

Native SP syntax:

```sp
fn add(int a, int b) -> int {
    return a + b;
}
```

Java/C#/C++-style compatibility:

```sp
int add(int a, int b) {
    return a + b;
}
```

Calling:

```sp
int result = add(10, 20);
```

---

# 11. Function Type Inference

SP can support:

```sp
fn add(a: int, b: int) -> int {
    return a + b;
}
```

Short form:

```sp
fn add(a, b) => a + b;
```

The short form should only be allowed when types can be safely inferred.

---

# 12. Control Flow

## If

```sp
if (age >= 18) {
    println("Adult");
}
```

## Else

```sp
if (age >= 18) {
    println("Adult");
} else {
    println("Minor");
}
```

## Else-if

```sp
if (score >= 80) {
    println("A");
} else if (score >= 70) {
    println("B");
} else {
    println("C");
}
```

---

# 13. Switch

C/Java/C# style:

```sp
switch (day) {
    case 1:
        println("Monday");
        break;

    case 2:
        println("Tuesday");
        break;

    default:
        println("Unknown");
}
```

Modern SP:

```sp
match day {
    1 => println("Monday"),
    2 => println("Tuesday"),
    _ => println("Unknown")
}
```

Both can be supported.

---

# 14. Loops

## For

```sp
for (int i = 0; i < 10; i++) {
    println(i);
}
```

## While

```sp
while (condition) {
    // code
}
```

## Do While

```sp
do {
    // code
} while (condition);
```

## Range loop

Modern SP:

```sp
for (i in 0..10) {
    println(i);
}
```

---

# 15. Arrays

```sp
int[] numbers = {1, 2, 3, 4, 5};
```

Access:

```sp
println(numbers[0]);
```

Modern syntax:

```sp
let numbers = [1, 2, 3, 4, 5];
```

---

# 16. Lists

```sp
List<int> numbers = new List<int>();

numbers.add(10);
numbers.add(20);
numbers.add(30);
```

Modern SP:

```sp
let numbers = [10, 20, 30];

numbers.push(40);
```

---

# 17. Maps

```sp
Map<string, int> ages = new Map<string, int>();

ages["Shanto"] = 21;
```

Modern syntax:

```sp
let ages = {
    "Shanto": 21,
    "Padmasree": 21
};
```

---

# 18. Classes

Java/C# style:

```sp
class User {
    string name;
    int age;

    User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    void sayHello() {
        println("Hello " + name);
    }
}
```

Create object:

```sp
User user = new User("Shanto", 21);

user.sayHello();
```

---

# 19. Access Modifiers

SP should support:

```text
public
private
protected
internal
```

Example:

```sp
class User {
    private string password;

    public string name;

    protected int id;
}
```

---

# 20. Constructors

```sp
class User {

    string name;

    constructor(string name) {
        this.name = name;
    }
}
```

Java/C# compatibility:

```sp
class User {

    string name;

    User(string name) {
        this.name = name;
    }
}
```

---

# 21. Inheritance

```sp
class Animal {
    fn speak() {
        println("Animal sound");
    }
}

class Dog extends Animal {
    override fn speak() {
        println("Woof");
    }
}
```

C# style can also be accepted:

```sp
class Dog : Animal {
}
```

SP compiler should normalize both forms into the same AST.

---

# 22. Interfaces

```sp
interface Printable {
    fn print();
}
```

Implementation:

```sp
class Document implements Printable {

    fn print() {
        println("Document");
    }
}
```

C#-style syntax may also be accepted:

```sp
class Document : Printable {
}
```

---

# 23. Abstract Classes

```sp
abstract class Animal {

    abstract fn speak();

}
```

---

# 24. Static Members

```sp
class Math {

    static fn square(int x) -> int {
        return x * x;
    }
}
```

Call:

```sp
int result = Math.square(10);
```

---

# 25. Generics

SP should support generic programming.

```sp
class Box<T> {

    T value;

    Box(T value) {
        this.value = value;
    }

    T getValue() {
        return value;
    }
}
```

Usage:

```sp
Box<int> box = new Box<int>(100);
```

Generic function:

```sp
fn max<T>(T a, T b) -> T {
    // generic implementation
}
```

---

# 26. Struct

SP can support lightweight value types:

```sp
struct Point {
    int x;
    int y;
}
```

Usage:

```sp
Point p;

p.x = 10;
p.y = 20;
```

---

# 27. Enum

```sp
enum Status {
    Pending,
    Active,
    Completed,
    Failed
}
```

Usage:

```sp
Status status = Status.Active;
```

---

# 28. Null Safety

SP should avoid unsafe null usage.

Instead of:

```sp
User user = null;
```

SP should support:

```sp
User? user = null;
```

Safe access:

```sp
println(user?.name);
```

Null fallback:

```sp
string name = user?.name ?? "Unknown";
```

This is a major modern feature.

---

# 29. Error Handling

SP should support exceptions.

```sp
try {
    riskyOperation();
}
catch (Exception e) {
    println(e.message);
}
finally {
    cleanup();
}
```

Modern SP could additionally support:

```sp
Result<T, E>
```

Example:

```sp
fn divide(int a, int b) -> Result<int, Error> {

    if (b == 0) {
        return Error("Cannot divide by zero");
    }

    return Ok(a / b);
}
```

---

# 30. Modules

Example:

```sp
module myapp.users;
```

Import:

```sp
import myapp.users.User;
```

Or:

```sp
import myapp.users.*;
```

Standard library:

```sp
import std.io;
import std.math;
import std.collections;
```

---

# 31. Package System

SP should have a package manager named:

```text
spm
```

Example:

```bash
spm init
```

Install package:

```bash
spm add http
```

Remove:

```bash
spm remove http
```

Update:

```bash
spm update
```

Build:

```bash
spm build
```

Run:

```bash
spm run
```

---

# 32. Project Configuration

Every SP project can contain:

```text
sp.toml
```

Example:

```toml
[package]
name = "hello-world"
version = "1.0.0"
authors = ["Shanto Paul"]

[language]
version = "1.0"

[dependencies]
http = "1.2.0"
json = "2.0.0"

[build]
entry = "src/main.sp"
```

---

# 33. Comments

Single line:

```sp
// This is a comment
```

Multi-line:

```sp
/*
    This is a
    multi-line comment
*/
```

Documentation:

```sp
/// Calculates the sum of two numbers.
fn add(int a, int b) -> int {
    return a + b;
}
```

---

# 34. Operators

Arithmetic:

```text
+
-
*
/
%
```

Comparison:

```text
==
!=
>
<
>=
<=
```

Logical:

```text
&&
||
!
```

Bitwise:

```text
&
|
^
~
<<
>>
```

Assignment:

```text
=
+=
-=
*=
/=
%=
```

Increment:

```text
++
--
```

---

# 35. String System

```sp
string name = "Shanto";
```

Concatenation:

```sp
string message = "Hello " + name;
```

String interpolation:

```sp
string message = "Hello ${name}";
```

Methods:

```sp
name.length();
name.upper();
name.lower();
name.contains("an");
```

---

# 36. Lambda Functions

```sp
var add = (int a, int b) => a + b;
```

Usage:

```sp
println(add(10, 20));
```

Higher-order functions:

```sp
numbers.map(x => x * 2);
```

---

# 37. Functional Features

SP should support:

```text
map
filter
reduce
foreach
lambda
closures
higher-order functions
```

Example:

```sp
let numbers = [1, 2, 3, 4, 5];

let doubled = numbers
    .map(x => x * 2)
    .filter(x => x > 5);
```

---

# 38. Async Programming

SP should eventually support:

```sp
async fn fetchData() {
    let response = await http.get(url);
    println(response);
}
```

This allows asynchronous applications without complicated callback systems.

---

# 39. Concurrency

SP should support:

```sp
thread {
    println("Running in thread");
}
```

Future versions may include:

```sp
async
await
spawn
channel
mutex
atomic
parallel
```

---

# 40. Memory Model

The language should hide unnecessary memory-management complexity from normal users.

Recommended model:

```text
Primitive values
    ↓
Stack/value storage

Objects
    ↓
Managed heap

Resources
    ↓
Automatic cleanup / RAII
```

SP should eventually provide explicit low-level control for systems programmers.

Example:

```sp
unsafe {
    // low-level operations
}
```

---

# 41. Unsafe Mode

For systems programming:

```sp
unsafe {
    pointer p = ...
}
```

Unsafe APIs should be isolated.

Normal SP programs should remain memory-safe.

---

# 42. Native Interoperability

SP should eventually communicate with C libraries.

Example:

```sp
extern "C" {
    fn printf(string format);
}
```

This enables integration with:

```text
C
C++
Operating System APIs
Native libraries
Hardware libraries
```

---

# 43. Java Interoperability

Java compatibility should NOT mean SP automatically executes Java bytecode.

Instead, future SP versions may provide:

```text
Java JNI / JVM bridge
```

Possible syntax:

```sp
java.import("java.util.ArrayList");
```

This should be considered an advanced interoperability feature.

---

# 44. C# Interoperability

Future versions may support .NET interoperability.

Architecture:

```text
SP
 ↓
.NET Interop Layer
 ↓
C# Libraries
```

This could allow SP applications to call selected .NET APIs.

---

# 45. C++ Interoperability

C++ interoperability is significantly harder.

SP should initially use:

```text
C ABI
```

rather than attempting full C++ ABI compatibility.

Later:

```text
SP
 ↓
C++ Bridge
 ↓
C++ Libraries
```

could be implemented.

---

# 46. Important Compatibility Rule

SP should support familiar syntax from:

```text
C
C++
Java
C#
```

but it should NOT promise:

```text
100% Java source compatibility
100% C# source compatibility
100% C++ source compatibility
```

Those languages have different:

- type systems
- object models
- memory models
- generics
- exceptions
- runtime systems
- ABI models

Therefore SP should use:

```text
Syntax Compatibility
+
Concept Compatibility
+
Interop
```

instead of pretending that all four languages are identical.

---

# 47. Syntax Compatibility Layer

The compiler frontend can normalize multiple syntax styles.

Example 1:

```sp
int add(int a, int b) {
    return a + b;
}
```

Example 2:

```sp
fn add(int a, int b) -> int {
    return a + b;
}
```

Both become:

```text
FunctionNode
├── name: add
├── parameters:
│   ├── a: int
│   └── b: int
├── returnType: int
└── body:
    └── return a + b
```

This is one of the most important concepts in SP's compiler.

---

# 48. Compiler Architecture

The SP compiler should have the following architecture:

```text
                  ┌───────────────┐
                  │  source.sp    │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │    Lexer      │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │    Parser     │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │     AST       │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │ Name Resolver │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │ Type Checker  │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │   IR Builder  │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │  LLVM IR      │
                  └───────┬───────┘
                          │
                          ▼
                  ┌───────────────┐
                  │ LLVM Backend  │
                  └───────┬───────┘
                          │
                          ▼
                 Native Executable
```

---

# 49. Lexer

The lexer converts characters into tokens.

Example:

```sp
int age = 20;
```

becomes approximately:

```text
KEYWORD_INT
IDENTIFIER(age)
ASSIGN
INTEGER(20)
SEMICOLON
```

The lexer should recognize:

```text
keywords
identifiers
numbers
strings
characters
operators
comments
punctuation
```

---

# 50. Parser

The parser converts tokens into an AST.

Example:

```sp
int x = 10 + 20;
```

AST:

```text
VariableDeclaration
├── type: int
├── name: x
└── initializer
    └── BinaryExpression
        ├── left: 10
        ├── operator: +
        └── right: 20
```

---

# 51. AST

The SP compiler should define AST nodes such as:

```text
Program
Module
Import
ClassDeclaration
InterfaceDeclaration
StructDeclaration
EnumDeclaration
FunctionDeclaration
VariableDeclaration
Parameter
BlockStatement
IfStatement
ForStatement
WhileStatement
SwitchStatement
ReturnStatement
BreakStatement
ContinueStatement
BinaryExpression
UnaryExpression
CallExpression
MemberExpression
LiteralExpression
ArrayExpression
LambdaExpression
NewExpression
```

---

# 52. Semantic Analysis

After parsing, SP needs semantic analysis.

The compiler checks:

```text
variable existence
duplicate declarations
type compatibility
function overloads
generic constraints
inheritance
interface implementation
access modifiers
return types
null safety
scope
```

Example:

```sp
int age = "hello";
```

Compiler:

```text
error: cannot assign value of type 'string'
to variable of type 'int'
```

---

# 53. Type Checker

The type checker should understand:

```text
primitive types
classes
interfaces
structs
arrays
generics
nullable types
function types
union/result types
```

---

# 54. Intermediate Representation

SP should have its own high-level intermediate representation before LLVM.

Architecture:

```text
SP Source
   ↓
Tokens
   ↓
AST
   ↓
Typed AST
   ↓
SP IR
   ↓
LLVM IR
   ↓
Machine Code
```

This makes the compiler easier to evolve.

---

# 55. LLVM

LLVM is recommended for the first serious SP compiler backend.

Advantages:

```text
x86-64
ARM64
RISC-V
Windows
Linux
macOS
optimization
debug information
native binaries
```

This allows SP to focus on language design while LLVM handles much of the machine-code backend.

---

# 56. Compiler CLI

The main command:

```bash
spc
```

Compile:

```bash
spc main.sp
```

Output:

```text
main.exe
```

Linux:

```bash
spc main.sp -o main
```

Run:

```bash
spc run main.sp
```

Build:

```bash
spc build
```

Check:

```bash
spc check
```

Format:

```bash
spc fmt
```

Test:

```bash
spc test
```

Documentation:

```bash
spc doc
```

Version:

```bash
spc --version
```

---

# 57. Recommended CLI Structure

```text
spc
├── build
├── run
├── check
├── test
├── fmt
├── doc
├── clean
├── init
├── package
└── version
```

Package manager:

```text
spm
├── init
├── add
├── remove
├── update
├── install
├── publish
└── search
```

---

# 58. Standard Library

SP should eventually provide:

```text
std.io
std.fs
std.math
std.string
std.collections
std.time
std.json
std.http
std.net
std.crypto
std.regex
std.concurrent
std.process
std.os
std.testing
```

Example:

```sp
import std.io;

fn main() {
    println("Hello SP");
}
```

---

# 59. HTTP

Example future API:

```sp
import std.http;

fn main() async {

    let response = await http.get(
        "https://example.com"
    );

    println(response.body);
}
```

---

# 60. JSON

```sp
import std.json;

let user = json.parse<User>(data);

println(user.name);
```

---

# 61. File System

```sp
import std.fs;

let content = fs.readText("data.txt");

fs.writeText(
    "output.txt",
    "Hello SP"
);
```

---

# 62. Testing

SP should have a built-in testing system.

Example:

```sp
test "addition works" {

    assert(add(2, 3) == 5);

}
```

Run:

```bash
spc test
```

---

# 63. Package Registry

Future registry:

```text
registry.splang.shantopaul.com
```

Example:

```bash
spm search http
```

Install:

```bash
spm add http-client
```

Publish:

```bash
spm publish
```

---

# 64. Code Formatter

SP should provide:

```bash
spc fmt
```

Input:

```sp
fn add( int a,int b ){return a+b;}
```

Formatted:

```sp
fn add(int a, int b) {
    return a + b;
}
```

---

# 65. Language Server

SP should eventually have:

```text
sp-language-server
```

Features:

```text
autocomplete
go to definition
go to declaration
rename symbol
diagnostics
documentation
code actions
formatting
refactoring
```

This enables VS Code support.

---

# 66. VS Code Extension

The official extension:

```text
SP Language Support
```

File association:

```text
*.sp
```

Features:

```text
syntax highlighting
autocomplete
compiler errors
formatting
debugging
project support
```

---

# 67. Debugger

SP should support debugging through:

```text
DAP
```

Debugging features:

```text
breakpoints
step over
step into
step out
watch variables
call stack
exception inspection
```

---

# 68. Error Messages

Error messages should be human-friendly.

Bad:

```text
ERR_0x183A
```

Good:

```text
error[E1001]: type mismatch

  --> src/main.sp:8:14

8 | int age = "21";
  |           ^^^^

expected: int
found:    string

help: convert the string to an integer
```

---

# 69. Error Code System

Example:

```text
E1000 Syntax Error
E1001 Type Error
E1002 Undefined Variable
E1003 Undefined Function
E1004 Duplicate Declaration
E1005 Invalid Return Type
E1006 Access Violation
E1007 Generic Constraint Error
E1008 Null Safety Error
E1009 Import Error
```

---

# 70. Project Structure

The SP compiler repository:

```text
sp-language/
│
├── compiler/
│   ├── lexer/
│   ├── parser/
│   ├── ast/
│   ├── semantic/
│   ├── typechecker/
│   ├── ir/
│   ├── codegen/
│   └── diagnostics/
│
├── runtime/
│
├── stdlib/
│   ├── io/
│   ├── fs/
│   ├── math/
│   ├── string/
│   ├── collections/
│   ├── http/
│   └── json/
│
├── tools/
│   ├── spc/
│   ├── spm/
│   └── formatter/
│
├── language-server/
│
├── vscode-extension/
│
├── tests/
│
├── examples/
│
├── docs/
│
├── benchmarks/
│
├── LICENSE
├── README.md
├── CONTRIBUTING.md
└── ROADMAP.md
```

---

# 71. Compiler Internal Structure

A possible C++ implementation:

```text
compiler/
│
├── include/
│   └── sp/
│       ├── lexer/
│       ├── parser/
│       ├── ast/
│       ├── semantic/
│       ├── types/
│       ├── ir/
│       └── codegen/
│
└── src/
    ├── lexer/
    ├── parser/
    ├── ast/
    ├── semantic/
    ├── types/
    ├── ir/
    └── codegen/
```

---

# 72. Implementation Language

Recommended initial implementation:

## Option A — C++

```text
SP Compiler
      ↓
C++
      ↓
LLVM
```

Advantages:

- excellent LLVM integration
- fast compiler
- suitable for systems programming
- mature tooling

## Option B — Rust

```text
SP Compiler
      ↓
Rust
      ↓
LLVM
```

Advantages:

- memory safety
- modern compiler development
- excellent tooling

For the first version, either C++ or Rust is suitable.

---

# 73. Recommended Development Stack

```text
Language implementation: C++ or Rust
Backend: LLVM
Parser: Hand-written recursive descent
Build system: CMake
Testing: GoogleTest / Catch2
Package format: TOML
Version control: Git
Repository: GitHub
Documentation: Markdown
CI: GitHub Actions
Editor: VS Code
```

---

# 74. Parser Strategy

A hand-written recursive descent parser is recommended for the first compiler.

Example:

```text
parseProgram()
parseDeclaration()
parseClass()
parseFunction()
parseStatement()
parseExpression()
parsePrimary()
```

Expression parsing can use:

```text
Pratt Parser
```

This makes operator precedence easier to manage.

---

# 75. Grammar

A simplified grammar:

```ebnf
program
    = declaration* ;

declaration
    = function
    | class
    | interface
    | struct
    | enum
    | variable ;

function
    = "fn" identifier
      "(" parameters? ")"
      returnType?
      block ;

block
    = "{" statement* "}" ;

statement
    = variableDeclaration
    | expressionStatement
    | ifStatement
    | whileStatement
    | forStatement
    | returnStatement
    | block ;

expression
    = assignment ;

assignment
    = logicalOr
      ("=" assignment)? ;

logicalOr
    = logicalAnd
      ("||" logicalAnd)* ;

logicalAnd
    = equality
      ("&&" equality)* ;

equality
    = comparison
      (("==" | "!=") comparison)* ;

comparison
    = term
      ((">" | "<" | ">=" | "<=") term)* ;

term
    = factor
      (("+" | "-") factor)* ;

factor
    = unary
      (("*" | "/" | "%") unary)* ;

unary
    = ("!" | "-" | "+") unary
    | primary ;

primary
    = literal
    | identifier
    | call
    | "(" expression ")" ;
```

This grammar should evolve as SP matures.

---

# 76. Syntax Compatibility Architecture

The parser should have a compatibility layer.

```text
                 Source Code
                     │
         ┌───────────┼───────────┐
         │           │           │
     SP Style    Java Style   C# Style
         │           │           │
         └───────────┼───────────┘
                     │
                     ▼
              Common AST
                     │
                     ▼
             Semantic Analysis
```

C++ syntax can also be selectively supported.

---

# 77. Example: Multiple Syntax Styles

### SP

```sp
fn add(int a, int b) -> int {
    return a + b;
}
```

### Java/C#/C++

```sp
int add(int a, int b) {
    return a + b;
}
```

The compiler should produce the same semantic representation.

---

# 78. Main Program

Recommended:

```sp
fn main() {

    println("Hello from SP");

}
```

Alternative compatibility form:

```sp
public static void main() {

    System.out.println("Hello from SP");

}
```

However, `System.out.println` should only work if SP defines a compatible `System` API.

The compiler should not magically import Java's runtime.

---

# 79. Example Complete Application

```sp
import std.io;

class User {

    private string name;
    private int age;

    public User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public fn introduce() {
        println(
            "My name is ${name} and I am ${age} years old."
        );
    }
}

fn main() {

    User user = new User("Shanto", 21);

    user.introduce();

}
```

---

# 80. Compiler Execution

Command:

```bash
spc build src/main.sp
```

Compiler pipeline:

```text
main.sp
 ↓
Lexing
 ↓
Parsing
 ↓
AST
 ↓
Semantic Analysis
 ↓
Type Checking
 ↓
SP IR
 ↓
LLVM IR
 ↓
Optimization
 ↓
Machine Code
 ↓
Executable
```

---

# 81. Build Modes

SP should provide:

```text
debug
release
test
profile
```

Example:

```bash
spc build --debug
```

```bash
spc build --release
```

Release mode:

```text
optimization
dead-code elimination
constant folding
inlining
LLVM optimizations
```

---

# 82. Cross Compilation

Future:

```bash
spc build --target windows-x64
spc build --target linux-x64
spc build --target linux-arm64
spc build --target macos-arm64
```

Targets:

```text
Windows x64
Linux x64
Linux ARM64
macOS ARM64
macOS x64
```

Future:

```text
Android
iOS
WebAssembly
```

---

# 83. WebAssembly

A future SP compiler target:

```text
SP
 ↓
LLVM/WASM
 ↓
WebAssembly
```

This could allow:

```sp
fn add(int a, int b) -> int {
    return a + b;
}
```

to run inside web applications.

---

# 84. Garbage Collection

If SP uses GC, the runtime should contain:

```text
allocator
heap manager
collector
object metadata
root tracking
```

Possible initial implementation:

```text
mark-and-sweep
```

Later:

```text
generational GC
concurrent GC
compact GC
```

---

# 85. Alternative Memory Strategy

Another possibility is:

```text
ownership
borrowing
reference counting
RAII
```

This is more complex but provides stronger systems-level control.

A practical roadmap is:

```text
SP 0.x
managed memory

SP 1.x
optimized memory runtime

SP 2.x
advanced ownership/unsafe features
```

---

# 86. Security

The language ecosystem should include:

```text
package signing
dependency lock files
checksum verification
sandboxing
secure defaults
memory safety
```

Package lock:

```text
sp.lock
```

---

# 87. Versioning

SP should use semantic versioning:

```text
MAJOR.MINOR.PATCH
```

Example:

```text
1.0.0
1.1.0
1.1.1
2.0.0
```

Language versions:

```text
SP 0.1
SP 0.2
SP 0.5
SP 0.9
SP 1.0
```

---

# 88. Compatibility Policy

Once SP reaches 1.0:

```text
SP 1.x
```

should maintain strong source compatibility.

Breaking changes should require:

```text
SP 2.0
```

---

# 89. Standard Library Philosophy

The standard library should remain:

```text
small
fast
well-tested
cross-platform
stable
```

Third-party packages should provide specialized functionality.

---

# 90. SP Runtime

The runtime may contain:

```text
memory management
threading
async runtime
exceptions
reflection
object system
I/O
process management
FFI
```

Architecture:

```text
SP Application
      ↓
SP Standard Library
      ↓
SP Runtime
      ↓
Operating System
```

---

# 91. Reflection

Future SP versions could support:

```sp
typeOf(user);
```

and:

```sp
reflection.fields<User>();
```

Useful for:

```text
ORM
serialization
dependency injection
web frameworks
testing
```

---

# 92. Annotations

SP should support annotations:

```sp
@Serializable
class User {
    string name;
    int age;
}
```

Other examples:

```sp
@Test
@Deprecated
@Route("/users")
@Entity
```

---

# 93. Attributes

For compiler metadata:

```sp
@inline
fn add(int a, int b) -> int {
    return a + b;
}
```

---

# 94. Operator Overloading

SP may support controlled operator overloading.

Example:

```sp
struct Vector2 {

    float x;
    float y;

    operator +(Vector2 a, Vector2 b) {
        return Vector2(
            a.x + b.x,
            a.y + b.y
        );
    }
}
```

This should be carefully restricted to avoid unreadable code.

---

# 95. Method Overloading

```sp
fn print(int value) {
}

fn print(string value) {
}

fn print(double value) {
}
```

Compiler resolves the correct overload using types.

---

# 96. Extension Methods

Future feature:

```sp
extend string {

    fn isEmail() -> bool {
        // implementation
    }

}
```

Usage:

```sp
if (email.isEmail()) {
}
```

---

# 97. Traits

A future SP feature can combine interface-like and reusable behavior:

```sp
trait Serializable {
    fn serialize();
}
```

A class can implement:

```sp
class User with Serializable {
}
```

---

# 98. Database Support

The standard library should not contain a full ORM initially.

Instead, third-party libraries can provide:

```text
SP PostgreSQL
SP MySQL
SP SQLite
SP MongoDB
```

Example future API:

```sp
import db.mysql;

let db = MySQL.connect(config);

let users = db.query<User>(
    "SELECT * FROM users"
);
```

---

# 99. Web Framework Potential

Once SP is stable, an official web framework could be built:

```text
SP Web
```

Example:

```sp
import sp.web;

route.get("/hello", fn(request) {

    return "Hello SP";

});
```

---

# 100. CLI Application Support

SP should be excellent for CLI programs.

Example:

```sp
fn main(args: string[]) {

    for (arg in args) {
        println(arg);
    }

}
```

---

# 101. GUI Applications

Future ecosystem:

```text
SP GUI
```

Potential backends:

```text
Windows
macOS
Linux
Qt
native UI
WebView
```

---

# 102. Game Development

SP could eventually support:

```text
graphics
audio
input
physics
OpenGL
Vulkan
DirectX
```

But these should NOT be priorities during the first language release.

---

# 103. SP Language Roadmap

## Phase 0 — Research

Study:

```text
C
C++
Java
C#
Rust
Go
Swift
Kotlin
LLVM
compiler architecture
```

Deliverables:

```text
LANGUAGE_DESIGN.md
SYNTAX.md
TYPE_SYSTEM.md
COMPILER_ARCHITECTURE.md
```

---

# 104. Phase 1 — Lexer

Implement:

```text
keywords
identifiers
numbers
strings
characters
operators
comments
```

First successful output:

```text
tokens.sp
```

---

# 105. Phase 2 — Parser

Implement:

```text
expressions
variables
functions
blocks
if
while
for
return
```

Goal:

```text
source.sp → AST
```

---

# 106. Phase 3 — Semantic Analyzer

Implement:

```text
scope
symbol table
type checking
name resolution
return validation
```

---

# 107. Phase 4 — Basic Compiler

Implement:

```text
integer
float
boolean
string
functions
control flow
```

Target:

```text
LLVM
```

---

# 108. Phase 5 — Object System

Implement:

```text
class
object
constructor
methods
inheritance
interface
visibility
```

---

# 109. Phase 6 — Standard Library

Start with:

```text
io
string
math
collections
filesystem
```

---

# 110. Phase 7 — Package Manager

Implement:

```text
spm init
spm add
spm install
spm remove
spm update
spm publish
```

---

# 111. Phase 8 — Developer Tools

Implement:

```text
formatter
language server
VS Code extension
debugger
documentation generator
```

---

# 112. Phase 9 — Production Release

Target:

```text
SP 1.0
```

Requirements:

```text
stable compiler
stable language specification
standard library
package manager
documentation
Windows support
Linux support
macOS support
IDE support
test framework
debugging
```

---

# 113. Version Roadmap

```text
SP 0.1
Lexer + Parser

SP 0.2
AST + Semantic Analysis

SP 0.3
Basic LLVM compiler

SP 0.4
Functions + Classes

SP 0.5
Generics + Collections

SP 0.6
Standard Library

SP 0.7
Package Manager

SP 0.8
Language Server

SP 0.9
Production Candidate

SP 1.0
Stable Language
```

---

# 114. First MVP

Do NOT try to build everything immediately.

The first MVP should support:

```text
.sp files
variables
primitive types
functions
if/else
for
while
arrays
strings
classes
basic objects
imports
compiler
native executable
```

Example:

```sp
fn main() {

    int a = 10;
    int b = 20;

    int result = a + b;

    println(result);

}
```

The compiler should produce:

```text
main.exe
```

That is the first major milestone.

---

# 115. First Compiler Milestone

The first successful SP compiler should be able to execute:

```sp
fn main() {
    println("Hello, SP!");
}
```

Command:

```bash
spc run hello.sp
```

Output:

```text
Hello, SP!
```

Then:

```bash
spc build hello.sp
```

Output:

```text
hello.exe
```

---

# 116. Development Order

The recommended order is:

```text
1. Language specification
2. Token specification
3. Lexer
4. Parser
5. AST
6. Symbol table
7. Type checker
8. Semantic analyzer
9. Basic interpreter/compiler
10. LLVM backend
11. Runtime
12. Standard library
13. Classes
14. Generics
15. Modules
16. Package manager
17. Formatter
18. Language server
19. VS Code extension
20. Debugger
21. Documentation
22. SP 1.0
```

---

# 117. Testing Strategy

Compiler tests:

```text
lexer tests
parser tests
AST tests
type checker tests
semantic tests
code generation tests
runtime tests
integration tests
```

Example:

```text
tests/
├── lexer/
├── parser/
├── semantic/
├── typechecker/
├── codegen/
├── runtime/
├── integration/
└── examples/
```

---

# 118. Golden Tests

Example:

```text
input:
    int x = 10;

expected AST:
    VariableDeclaration(
        type=int,
        name=x,
        value=10
    )
```

Compiler output should be compared automatically.

---

# 119. Benchmarking

SP should eventually benchmark:

```text
compile time
startup time
runtime speed
memory usage
binary size
```

Benchmark programs:

```text
Fibonacci
Sorting
JSON parsing
HTTP
matrix multiplication
file processing
concurrency
```

Do not claim SP is faster than Java/C#/C++ until measured under controlled conditions.

---

# 120. Documentation Structure

The project documentation should eventually contain:

```text
docs/
│
├── getting-started/
│   ├── installation.md
│   ├── hello-world.md
│   └── first-project.md
│
├── language/
│   ├── syntax.md
│   ├── variables.md
│   ├── types.md
│   ├── functions.md
│   ├── classes.md
│   ├── generics.md
│   ├── interfaces.md
│   ├── errors.md
│   └── concurrency.md
│
├── compiler/
│   ├── architecture.md
│   ├── lexer.md
│   ├── parser.md
│   ├── ast.md
│   ├── typechecker.md
│   └── llvm.md
│
├── tooling/
│   ├── spc.md
│   ├── spm.md
│   ├── formatter.md
│   └── language-server.md
│
└── reference/
    └── language-reference.md
```

---

# 121. Official Documentation Website

Future documentation:

```text
https://splang.shantopaul.com/docs
```

Sections:

```text
Learn
Reference
Compiler
Standard Library
Packages
Tools
Contributing
Specification
```

---

# 122. SP Naming

Official branding:

```text
SP
```

Full name:

```text
SP Programming Language
```

Compiler:

```text
SPC
```

Package manager:

```text
SPM
```

Runtime:

```text
SPR
```

Language server:

```text
SPLS
```

---

# 123. Example Final SP Project

```text
todo-app/
│
├── sp.toml
│
├── src/
│   ├── main.sp
│   ├── models/
│   │   └── task.sp
│   ├── services/
│   │   └── task_service.sp
│   └── utils/
│       └── validation.sp
│
├── tests/
│   └── task_test.sp
│
└── README.md
```

---

# 124. Example Application

```sp
import std.io;
import std.collections;

class Task {

    private string title;
    private bool completed;

    public Task(string title) {
        this.title = title;
        this.completed = false;
    }

    public fn complete() {
        this.completed = true;
    }

    public fn getTitle() -> string {
        return this.title;
    }

    public fn isCompleted() -> bool {
        return this.completed;
    }
}

fn main() {

    List<Task> tasks = new List<Task>();

    tasks.add(
        new Task("Learn SP")
    );

    tasks.add(
        new Task("Build compiler")
    );

    for (Task task in tasks) {
        println(task.getTitle());
    }

}
```

---

# 125. What SP Should Ultimately Become

The long-term architecture:

```text
                       SP ECOSYSTEM
                            │
       ┌────────────────────┼────────────────────┐
       │                    │                    │
     Language            Compiler             Runtime
       │                    │                    │
       │              ┌─────┴─────┐              │
       │              │           │              │
      Syntax          AST        LLVM            GC
      Types           IR         Native          Async
      Classes         Codegen    Binary          Threads
      Generics        Optimizer  WASM            FFI
       │
       └────────────────────┬────────────────────┘
                            │
                       Standard Library
                            │
          ┌─────────────────┼─────────────────┐
          │                 │                 │
         IO                HTTP              JSON
          │                 │                 │
      Collections          FS                Math
                            │
                            ▼
                       Package Manager
                            │
                            ▼
                      Developer Tools
                            │
        ┌───────────────────┼──────────────────┐
        │                   │                  │
      VS Code             LSP              Debugger
        │                   │                  │
        └───────────────────┼──────────────────┘
                            │
                            ▼
                     SP Developer Ecosystem
```

---

# 126. SP's Unique Position

The main idea is:

```text
C++ familiarity
+
Java familiarity
+
C# familiarity
+
Modern safety
+
Modern tooling
+
Native performance
=
SP
```

But SP should not promise to literally become:

```text
C++ + Java + C#
```

Instead:

```text
Familiar syntax
+
Unified language design
+
Native compiler
+
Modern standard library
+
Strong tooling
=
SP
```

---

# 127. Critical Technical Decision

SP should use one canonical internal representation.

For example:

```text
Java-style input
        │
C#-style input ──→ SP AST ──→ Type System ──→ LLVM
        │
C++-style input
        │
SP-style input
```

This is how multiple syntax styles can coexist without creating four separate compilers.

---

# 128. What NOT To Do

Do not begin by attempting:

```text
100% C++ compatibility
100% Java compatibility
100% C# compatibility
mobile apps
web apps
desktop GUI
game engine
database
AI framework
package ecosystem
IDE
```

all at once.

That would make the project extremely difficult.

Start with:

```text
Lexer
→ Parser
→ AST
→ Type Checker
→ LLVM
→ Basic Standard Library
```

Then expand.

---

# 129. Recommended First 10 SP Programs

The compiler should eventually pass these examples:

## Program 1

```sp
fn main() {
    println("Hello SP");
}
```

## Program 2

```sp
fn main() {

    int a = 10;
    int b = 20;

    println(a + b);

}
```

## Program 3

```sp
fn main() {

    for (int i = 0; i < 10; i++) {
        println(i);
    }

}
```

## Program 4

```sp
fn add(int a, int b) -> int {
    return a + b;
}
```

## Program 5

```sp
class User {

    string name;

    User(string name) {
        this.name = name;
    }
}
```

## Program 6

```sp
interface Printable {

    fn print();

}
```

## Program 7

```sp
class Dog extends Animal {

    override fn speak() {
        println("Woof");
    }

}
```

## Program 8

```sp
let numbers = [1, 2, 3, 4, 5];

for (int n in numbers) {
    println(n);
}
```

## Program 9

```sp
async fn main() {

    let data = await fetchData();

    println(data);

}
```

## Program 10

```sp
fn main() {

    try {

        riskyOperation();

    } catch (Exception e) {

        println(e.message);

    }

}
```

---

# 130. First Official Specification Files

The SP repository should create these files first:

```text
README.md
LANGUAGE_SPEC.md
SYNTAX_SPEC.md
TYPE_SYSTEM.md
COMPILER_ARCHITECTURE.md
MEMORY_MODEL.md
STANDARD_LIBRARY.md
MODULE_SYSTEM.md
PACKAGE_MANAGER.md
ERROR_HANDLING.md
CONCURRENCY.md
FFI.md
ROADMAP.md
CONTRIBUTING.md
CODE_OF_CONDUCT.md
LICENSE
```

---

# 131. Initial Development Milestone

The first real milestone is:

```text
SP 0.1.0
```

Requirements:

```text
[ ] .sp source files
[ ] Lexer
[ ] Parser
[ ] AST
[ ] Variables
[ ] int
[ ] double
[ ] bool
[ ] string
[ ] arithmetic
[ ] if/else
[ ] while
[ ] for
[ ] functions
[ ] return
[ ] compiler errors
[ ] CLI
```

---

# 132. SP 0.5.0

Requirements:

```text
[ ] Classes
[ ] Objects
[ ] Constructors
[ ] Interfaces
[ ] Inheritance
[ ] Arrays
[ ] Collections
[ ] Generics
[ ] Modules
[ ] Imports
[ ] Standard library
```

---

# 133. SP 1.0.0

Requirements:

```text
[ ] Stable language specification
[ ] Native compiler
[ ] Windows
[ ] Linux
[ ] macOS
[ ] Standard library
[ ] Package manager
[ ] Formatter
[ ] Language Server
[ ] VS Code extension
[ ] Debugging
[ ] Testing
[ ] Documentation
[ ] Stable ABI/FFI
```

---

# 134. Final Architecture

The complete SP ecosystem should eventually look like:

```text
                         SP LANGUAGE
                              │
                 ┌────────────┴────────────┐
                 │                         │
             SP Compiler                 SP VM
                 │                         │
               LLVM                     Bytecode
                 │                         │
          Native Executable           SP Runtime
                 │                         │
       ┌─────────┼──────────┐              │
       │         │          │              │
    Windows    Linux      macOS           WASM
       │         │          │              │
       └─────────┼──────────┴──────────────┘
                 │
          Standard Library
                 │
          Package Manager
                 │
       ┌─────────┼───────────┐
       │         │           │
      CLI       LSP       Debugger
       │         │           │
       └─────────┼───────────┘
                 │
              IDEs
                 │
        ┌────────┼─────────┐
        │        │         │
      VS Code  JetBrains  Vim/Neovim
```

---

# 135. Final Product Definition

SP is a general-purpose programming language using `.sp` source files.

It provides a familiar syntax influenced by C, C++, Java and C#, while maintaining its own language semantics.

The core implementation should be:

```text
SP Source
    ↓
Lexer
    ↓
Parser
    ↓
AST
    ↓
Semantic Analyzer
    ↓
Type Checker
    ↓
SP IR
    ↓
LLVM IR
    ↓
Native Machine Code
```

The ecosystem should eventually contain:

```text
SP Compiler
SP Runtime
SP Standard Library
SP Package Manager
SP Language Server
SP Formatter
SP Debugger
SP Testing Framework
VS Code Extension
Documentation
Package Registry
```

The first objective is not to make SP compete with every established language immediately.

The first objective is to make this work reliably:

```sp
fn main() {
    println("Hello, SP!");
}
```

with:

```bash
spc run hello.sp
```

and:

```text
Hello, SP!
```

Once that foundation works, the language can progressively grow into a complete programming ecosystem.