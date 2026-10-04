# SP Syntax & Grammar Specification

This document defines the formal grammar and lexical rules of the **SP Programming Language**.

---

## 1. Lexical Elements

### 1.1 Keywords
The following tokens are reserved keywords in SP:
```text
fn, class, interface, struct, enum, trait,
if, else, for, while, do, switch, match, case, default, break, continue, return,
import, module, new, this, super, null, true, false,
var, let, const,
public, private, protected, internal, static, abstract, override, extends, implements, with,
async, await, try, catch, finally, throw, unsafe, extern,
test, assert, thread, spawn, channel, mutex,
void, int, uint, int8, int16, int32, int64,
uint8, uint16, uint32, uint64, float, double, bool, char, string, byte
```

### 1.2 Operators & Punctuators
- **Arithmetic:** `+`, `-`, `*`, `/`, `%`
- **Comparison:** `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Logical:** `&&`, `||`, `!`
- **Bitwise:** `&`, `|`, `^`, `~`, `<<`, `>>`
- **Assignment:** `=`, `+=`, `-=`, `*=`, `/=`, `%=`
- **Increment/Decrement:** `++`, `--`
- **Member & Flow:** `.`, `?.`, `??`, `->`, `=>`, `::`, `..`
- **Delimiters:** `(`, `)`, `{`, `}`, `[`, `]`, `;`, `:`, `,`

---

## 2. Formal Grammar (EBNF Subset)

```ebnf
program
    = moduleDeclaration? importDeclaration* topLevelDeclaration* ;

moduleDeclaration
    = "module" qualifiedIdentifier ";" ;

importDeclaration
    = "import" qualifiedIdentifier ( ".*" )? ";" ;

topLevelDeclaration
    = classDeclaration
    | interfaceDeclaration
    | structDeclaration
    | enumDeclaration
    | functionDeclaration
    | variableDeclaration ;

functionDeclaration
    = attribute* visibilityModifier? "static"? "override"?
      ( "fn" identifier "(" parameterList? ")" ( "->" type )? ( block | "=>" expression ";" )
      | type identifier "(" parameterList? ")" block ) ;

parameterList
    = parameter ( "," parameter )* ;

parameter
    = type identifier ( "=" expression )? ;

classDeclaration
    = visibilityModifier? "abstract"? "class" identifier
      ( ( "extends" | ":" ) identifier )?
      ( ( "implements" | ":" ) qualifiedIdentifierList )?
      ( "with" qualifiedIdentifierList )?
      "{" classMember* "}" ;

variableDeclaration
    = ( type | "var" | "let" | "const" ) identifier ( "=" expression )? ";" ;

statement
    = block
    | variableDeclaration
    | ifStatement
    | whileStatement
    | doWhileStatement
    | forStatement
    | switchStatement
    | matchStatement
    | returnStatement
    | breakStatement
    | continueStatement
    | tryCatchStatement
    | expressionStatement ;
```

---

## 3. Syntax Compatibility Normalization

SP accepts multiple traditional styles and unifies them in the AST:

1. **Functions:**
   - SP Native: `fn add(int a, int b) -> int { return a + b; }`
   - Classic: `int add(int a, int b) { return a + b; }`
   - Short form: `fn add(int a, int b) => a + b;`
   - All produce identical `FunctionDeclaration` AST nodes.

2. **Inheritance & Interfaces:**
   - Java style: `class Dog extends Animal implements Printable`
   - C# style: `class Dog : Animal, Printable`
   - Both produce identical base and interface bindings on `ClassDeclaration`.

3. **Branching:**
   - Both `switch (...) { case ...: }` and `match ... { pattern => ... }` compile to the same branch tree representation.
