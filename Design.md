# SP Programming Language - Visual Design System

This document covers the comprehensive visual design system for the **SP Programming Language**, encompassing the compiler's CLI output, VS Code extension syntax highlighting, documentation website, and overall brand identity.

## 1. Brand Identity

- **Name:** SP Programming Language
- **Short:** SP
- **Tagline:** "Familiar Syntax. Modern Power. Native Performance."
- **Logo Concept:** Clean, minimal, modern — the letters "SP" in a geometric/monospace style, emphasizing speed, precision, and clarity.
- **Brand Personality:** Professional, approachable, modern, fast, reliable.

---

## 2. Colors & Theme

### Primary Brand Colors

| Name | Hex Code | Preview (Conceptual) | Usage |
|------|----------|----------------------|-------|
| **SP Blue** | `#2563EB` | 🟦 | Primary brand color, links, buttons |
| **SP Dark** | `#1E293B` | ⬛ | Dark backgrounds, headers |
| **SP Light**| `#F8FAFC` | ⬜ | Light backgrounds |
| **SP Accent**| `#06B6D4` | 🩵 | Highlights, secondary actions |
| **SP Success**| `#10B981` | 🟩 | Success states, build passed |
| **SP Warning**| `#F59E0B` | 🟧 | Warnings |
| **SP Error**| `#EF4444` | 🟥 | Errors, build failed |
| **SP Info** | `#3B82F6` | 🟦 | Informational messages |

### Terminal/CLI Output Colors

The SP compiler is designed with developer experience (DX) in mind. Output is clean, structured, and relies on standard ANSI escape codes for broad compatibility.

| Element | Color | ANSI Code |
|---------|-------|-----------|
| **Error label** | Bold Red | `\033[1;31m` |
| **Warning label** | Bold Yellow | `\033[1;33m` |
| **Info label** | Bold Blue | `\033[1;34m` |
| **Hint label** | Bold Cyan | `\033[1;36m` |
| **Source file path** | Cyan | `\033[36m` |
| **Line number** | Blue | `\033[34m` |
| **Error caret** (`^^^^`) | Bold Red | `\033[1;31m` |
| **Code snippet** | White/Default | `\033[0m` |
| **Help text** | Green | `\033[32m` |
| **Build success** | Bold Green | `\033[1;32m` |
| **Build time** | Dim/Gray | `\033[2m` |
| **Token/Type names** | Bold White | `\033[1;37m` |

### VS Code Syntax Highlighting Theme

#### SP Dark Theme

| Token Type | Color | Hex |
|-----------|-------|-----|
| Keywords (`fn`, `class`, `if`, `for`, `while`, `return`) | Purple/Violet | `#C678DD` |
| Types (`int`, `string`, `bool`, `float`, `double`) | Cyan/Teal | `#56B6C2` |
| Strings | Green | `#98C379` |
| Numbers | Orange | `#D19A66` |
| Comments | Gray | `#5C6370` |
| Functions/Methods | Blue | `#61AFEF` |
| Variables | White/Light | `#ABB2BF` |
| Operators | White | `#ABB2BF` |
| Class names | Yellow/Gold | `#E5C07B` |
| Annotations (`@`) | Yellow | `#E5C07B` |
| Constants/Enums | Orange | `#D19A66` |
| Brackets/Parens | Rainbow or light gray | `#ABB2BF` |
| String interpolation `${}` | Cyan | `#56B6C2` |
| Boolean literals | Orange | `#D19A66` |
| `this`/`self` | Red/Italic | `#E06C75` |
| `null` | Red | `#E06C75` |

#### SP Light Theme

| Token Type | Color | Hex |
|-----------|-------|-----|
| Keywords | Purple | `#7C3AED` |
| Types | Teal | `#0D9488` |
| Strings | Green | `#16A34A` |
| Numbers | Orange | `#EA580C` |
| Comments | Gray | `#9CA3AF` |
| Functions | Blue | `#2563EB` |
| Variables | Dark | `#1F2937` |
| Operators | Dark Gray | `#374151` |
| Class names | Amber | `#D97706` |

---

## 3. Fonts & Typography

### Code / Editor
- **Primary:** JetBrains Mono (free, excellent for code, distinct features)
- **Fallback:** Fira Code, Cascadia Code, Source Code Pro
- **Size:** 14px (editor default)
- **Line height:** 1.5
- **Ligatures:** Enabled (for `->`, `=>`, `!=`, `<=`, `>=`, `&&`, `||`)

### Terminal / CLI
- **Font:** User's terminal default (typically monospace like Consolas, Menlo, or Cascadia Code).
- **Styling:** Rely strictly on ANSI colors and bold/dim attributes, avoiding reliance on custom fonts or icon fonts (unless explicitly enabled via a `--nerd-fonts` flag).

### Documentation Website
- **Headings:** Inter (or system sans-serif: San Francisco, Segoe UI, Roboto)
- **Body text:** Inter (or system sans-serif)
- **Code blocks:** JetBrains Mono

#### Website Typography Scale

| Element | Font | Size | Weight | Color |
|---------|------|------|--------|-------|
| H1 | Inter | 2.5rem | 800 (ExtraBold) | `#1E293B` |
| H2 | Inter | 2rem | 700 (Bold) | `#1E293B` |
| H3 | Inter | 1.5rem | 600 (SemiBold) | `#334155` |
| H4 | Inter | 1.25rem | 600 (SemiBold) | `#334155` |
| Body | Inter | 1rem (16px)| 400 (Regular) | `#475569` |
| Code inline | JetBrains Mono | 0.875rem | 400 | `#1E293B` on `#F1F5F9` |
| Code block | JetBrains Mono | 0.875rem | 400 | Theme colors on `#1E293B` |
| Sidebar nav | Inter | 0.875rem | 500 (Medium) | `#64748B` |
| Footer | Inter | 0.875rem | 400 | `#94A3B8` |

---

## 4. Website Design (sp-lang.org)

### Layout
- **Max content width:** 1200px
- **Sidebar width:** 280px (sticky on desktop)
- **Content area:** fluid, readable line lengths (max 65-75 characters per line)
- **Responsive breakpoints:** 640px, 768px, 1024px, 1280px

### Color Themes (UI)

#### Dark Mode
- **Background:** `#0F172A`
- **Surface (Cards, Nav):** `#1E293B`
- **Text:** `#E2E8F0`
- **Muted text:** `#94A3B8`
- **Border:** `#334155`

#### Light Mode
- **Background:** `#FFFFFF`
- **Surface (Cards, Nav):** `#F8FAFC`
- **Text:** `#1E293B`
- **Muted text:** `#64748B`
- **Border:** `#E2E8F0`

### Code Example Styling
- **Background:** `#1E293B` (dark) or `#F8FAFC` (light)
- **Border:** 1px solid `#334155` (dark) or `#E2E8F0` (light)
- **Border radius:** 8px
- **Padding:** 16px
- **Line numbers:** `#4B5563` (gray)
- **UX:** Copy button fixed to the top-right corner of the block.

---

## 5. Icon & Badge Design

- **File Icon for `.sp` files:** A vertical rectangular document icon, predominantly **SP Blue**, featuring the clean "SP" geometric typography in white.
- **Build Badges:**
  - Pass: 🟩 Green (`#10B981`)
  - Fail: 🟥 Red (`#EF4444`)
  - Building: 🟨 Yellow (`#F59E0B`)
- **Package Badge:** SP Blue background with the version number in white (e.g., `sp-core v1.2.0`).

---

## 6. SP CLI & Error Message Design

SP takes inspiration from modern compilers (like Rust) to provide helpful, visually structured terminal output.

### Design Principles for CLI:
1. **Clear Hierarchy:** Error > File path > Code snippet > Help text.
2. **Contextual snippets:** Always show the offending code with line numbers and a precise caret (`^`).
3. **Actionable advice:** Provide `help:` or `hint:` whenever the compiler can infer a fix.

### Visual Examples (Simulated ANSI Output)

#### 1. Type Error
```text
error[E1001]: type mismatch
  --> src/main.sp:8:14
   |
 8 | int age = "21";
   |           ^^^^
   = expected: int
     found:    string
   = help: convert the string to an integer using int.parse("21")
```
- `error[E1001]:`: Bold Red
- `src/main.sp:8:14`: Cyan
- `8 |`: Blue
- `^^^^`: Bold Red
- `expected` / `found`: Default white
- `= help:`: Green

#### 2. Syntax Error
```text
error[E0024]: missing semicolon
  --> src/utils/math.sp:4:20
   |
 4 |     return a + b
   |                 ^ expected `;`
   |
   = help: add a semicolon at the end of the statement
```

#### 3. Undefined Variable Error
```text
error[E2045]: cannot find value `count` in this scope
  --> src/loop.sp:12:15
   |
11 |     int counter = 0;
12 |     while (count < 10) {
   |            ^^^^^ not found in this scope
   |
   = hint: did you mean `counter`?
```
- `^^^^^`: Bold Red
- `= hint:`: Bold Cyan

#### 4. Warning (Unused Variable)
```text
warning[W104]: unused variable: `temp`
  --> src/calc.sp:45:9
   |
45 |     let temp = 42;
   |         ^^^^
   |
   = note: prefix with `_` to ignore: `_temp`
```
- `warning[W104]:`: Bold Yellow
- `^^^^`: Bold Yellow
- `= note:`: Bold Blue

#### 5. Build Output (Success)
```text
   Compiling sp-core v1.0.0 (src/lib.sp)
   Compiling sp-app v0.1.0 (src/main.sp)
    Finished debug build [unoptimized + debuginfo] in 0.34s
     Running `./target/debug/sp-app`
Hello, SP!
```
- `Compiling`: Bold Green
- `Finished`: Bold Green
- `Running`: Bold Green
- Time (`0.34s`): Dim/Gray

#### 6. Build Output (Failure)
```text
   Compiling sp-app v0.1.0 (src/main.sp)
error: could not compile `sp-app` due to 2 previous errors

    Failed debug build in 0.12s
```
- `error:`: Bold Red
- `Failed`: Bold Red

#### 7. Test Results
```text
   Compiling sp-app tests (src/main.sp)
     Running unittests src/main.sp

test result: ok. 4 passed; 0 failed; 0 ignored; 0 measured; 0 filtered out; finished in 0.02s
```
- `Running`: Bold Green
- `ok`: Bold Green
- `passed`: Bold Green
- `finished in 0.02s`: Dim/Gray
