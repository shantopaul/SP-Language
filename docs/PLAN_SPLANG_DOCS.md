# Implementation Plan: Official Documentation Website (`docs.splang.shantopaul.com`)

## 1. Executive Summary & Objective

The **Official SP Documentation Website** (`https://docs.splang.shantopaul.com`) is the primary technical resource for developers learning, adopting, and building software with the SP Programming Language. It provides an exhaustive, searchable, and interactive documentation experience inspired by industry standards such as Rust Docs (`doc.rust-lang.org`), Next.js Docs, and Swift Docs.

The project will be located in the root repository folder under `/splang-docs` and built with **Next.js 14+ (App Router)**, **TypeScript**, **Tailwind CSS**, and modern developer-oriented UI components.

---

## 2. Technical Stack & Architecture

- **Framework:** Next.js 14+ (App Router, Server Components + Client interactivity)
- **Language:** TypeScript (strict mode enabled)
- **Styling:** Tailwind CSS with `@tailwindcss/typography` for documentation formatting
- **Icons:** `lucide-react`
- **Syntax Highlighting:** Custom syntax highlighter component styled specifically for SP syntax (keywords, types, decorators, strings, numbers, comments)
- **Search System:** Fast, client-side fuzzy search (MiniSearch / Fuse.js) with keyboard navigation (`Ctrl+K` / `Cmd+K` global command palette)
- **State Management:** React hooks and Context for theme toggle, sidebar expansion, search state, and active heading tracking
- **Deployment & Export:** Support for static site export (`output: 'export'`) or standalone containerized Node.js runtime for seamless deployment to Vercel, Cloudflare Pages, or custom Docker/Nginx infrastructure under domain `docs.splang.shantopaul.com`.

---

## 3. Directory Structure (`/splang-docs`)

```text
splang-docs/
├── public/
│   ├── favicon.ico
│   ├── logo.svg
│   ├── og-image.png
│   └── search-index.json
├── src/
│   ├── app/
│   │   ├── globals.css
│   │   ├── layout.tsx                     # Root layout with navbar, theme provider, and search modal
│   │   ├── page.tsx                       # Docs portal landing & quick start overview
│   │   ├── getting-started/
│   │   │   ├── installation/page.tsx       # OS-specific installation guides (Windows, Linux, macOS)
│   │   │   ├── first-program/page.tsx      # Writing & running Hello World
│   │   │   ├── project-setup/page.tsx      # Scaffolding projects with spm
│   │   │   └── page.tsx
│   │   ├── syntax-guide/
│   │   │   ├── variables/page.tsx          # Variables, let/var, mutability
│   │   │   ├── data-types/page.tsx         # Primitives, collections, user types
│   │   │   ├── control-flow/page.tsx       # if/else, loops, match/switch
│   │   │   ├── functions/page.tsx          # Functions, parameters, return types
│   │   │   ├── oop/page.tsx                # Classes, inheritance, interfaces, structs, enums
│   │   │   ├── generics/page.tsx           # Generics, monomorphization, constraints
│   │   │   ├── lambdas/page.tsx            # Lambdas, closures, higher-order functions
│   │   │   ├── null-safety/page.tsx        # Null safety, safe navigation, coalescing
│   │   │   └── page.tsx
│   │   ├── stdlib/
│   │   │   ├── io/page.tsx                 # std.io module reference
│   │   │   ├── string/page.tsx             # std.string module reference
│   │   │   ├── math/page.tsx               # std.math module reference
│   │   │   ├── collections/page.tsx        # std.collections (List, Map, Set)
│   │   │   ├── fs/page.tsx                 # std.fs file system API
│   │   │   ├── time/page.tsx               # std.time clock & duration API
│   │   │   ├── os/page.tsx                 # std.os environment & platform API
│   │   │   ├── process/page.tsx            # std.process execution API
│   │   │   └── page.tsx
│   │   ├── tools/
│   │   │   ├── compiler-cli/page.tsx       # spc build, run, check, flags (-O0..-O3, --target)
│   │   │   ├── package-manager/page.tsx    # spm commands and lockfile security
│   │   │   ├── formatter/page.tsx          # spc fmt configuration and rules
│   │   │   ├── test-runner/page.tsx        # spc test syntax and filtering
│   │   │   ├── doc-generator/page.tsx      # spc doc generation from /// comments
│   │   │   ├── lsp-server/page.tsx         # spls language server configuration
│   │   │   ├── debugger/page.tsx           # spd DAP debugger usage and setup
│   │   │   ├── vscode-extension/page.tsx   # VS Code installation and shortcuts
│   │   │   └── page.tsx
│   │   ├── benchmarks/
│   │   │   ├── overview/page.tsx           # Comparative benchmarks vs C, C++, Rust, Java, Node, Python
│   │   │   └── methodology/page.tsx        # Profiling environment and reproducibility
│   │   └── search/
│   │       └── route.ts                    # Search API endpoint for index queries
│   ├── components/
│   │   ├── layout/
│   │   │   ├── Header.tsx                  # Top navigation bar with logo, version, search bar, links
│   │   │   ├── Sidebar.tsx                 # Expandable multi-level documentation navigation tree
│   │   │   ├── TableOfContents.tsx         # Dynamic On-This-Page right sidebar with scrollspy
│   │   │   └── Footer.tsx                  # Helpful feedback links, GitHub edit link, prev/next buttons
│   │   ├── search/
│   │   │   ├── SearchModal.tsx             # Cmd+K fuzzy search dialog with instant previews
│   │   │   └── SearchBar.tsx               # Header quick trigger input
│   │   ├── ui/
│   │   │   ├── CodeBlock.tsx               # SP syntax-highlighted code block with copy button
│   │   │   ├── Callout.tsx                 # Tip, Note, Important, Warning alert boxes
│   │   │   ├── TabGroup.tsx                # Tabbed code switchers (e.g., Windows / Linux / macOS)
│   │   │   ├── Breadcrumbs.tsx             # Hierarchy breadcrumbs trail
│   │   │   └── PrevNextNav.tsx             # Bottom navigation to adjacent doc pages
│   │   └── docs/
│   │       ├── ApiSignature.tsx            # Standard library function/type signature card
│   │       └── BenchmarkChart.tsx          # Performance comparison visualizer
│   ├── data/
│   │   ├── navigation.ts                   # Structured sidebar menu schema
│   │   ├── stdlib-index.ts                 # Standard library API documentation metadata
│   │   └── search-items.ts                 # Full-text searchable documents index
│   └── lib/
│       ├── search.ts                       # Fuzzy search algorithm and indexing logic
│       └── utils.ts                        # Styling and string utilities
├── next.config.mjs
├── package.json
├── postcss.config.mjs
├── tailwind.config.ts
└── tsconfig.json
```

---

## 4. Documentation Sitemap & Content Coverage

1. **Introduction & Quickstart:**
   - What is SP? Core philosophy: System safety, modern syntax, zero-overhead native compilation.
   - 3-minute quickstart guide with single-command install scripts.
2. **Complete SP Language Tour (46 Features):**
   - Direct interactive representation of all 46 verified syntax guide programs ([`SP-Syntax-Guide.md`](file:///k:/AndroidDev/tools/Project/Own%20Programming%20Language/SP-Language/SP-Syntax-Guide.md)).
   - Variables, types, operators, strings, control flow, functions, OOP, interfaces, generics, collections, lambdas, memory model.
3. **Standard Library API Documentation:**
   - Full signatures, parameter tables, exceptions/errors, and runnable code examples for all 8 stdlib modules (`std.io`, `std.string`, `std.math`, `std.collections`, `std.fs`, `std.time`, `std.os`, `std.process`).
4. **Toolchain Reference:**
   - Exhaustive documentation for `spc` (compiler), `spm` (package manager), `spls` (LSP server), and `spd` (DAP debugger).
5. **VS Code & IDE Guide:**
   - Setup guide for the official extension in `editors/vscode/`, configuring breakpoints, running inline tests, and LSP autocomplete.
6. **Performance & Benchmarks:**
   - Interactive breakdown of the 7-language benchmarks across Fibonacci, Quicksort, Hash Map, Binary Trees, and Mandelbrot.

---

## 5. Implementation Phases for `/splang-docs`

### Phase D1: Project Foundation & Tooling Setup
- [ ] Initialize `/splang-docs` directory with Next.js 14+ App Router, TypeScript, and Tailwind CSS.
- [ ] Configure `tsconfig.json`, `tailwind.config.ts`, and `@tailwindcss/typography`.
- [ ] Implement responsive root layout, dark/light theme switcher, and navigation header.
- [ ] Setup Lucide icon integration and modern typography styles.

### Phase D2: Core UI Components & Code Viewer
- [ ] Build `CodeBlock` component with custom syntax highlighting specifically tuned for SP tokens:
  - Keywords: `fn`, `class`, `interface`, `struct`, `enum`, `let`, `var`, `if`, `else`, `match`, `for`, `in`, `return`, `import`.
  - Types: `int`, `double`, `string`, `bool`, `void`, generic `<T>`.
  - Literals, operators, strings with `${interpolation}`, and `///` doc comments.
  - One-click copy code snippet button with visual feedback.
- [ ] Build `Callout` component supporting Note, Tip, Warning, and Important alerts.
- [ ] Build `TabGroup` component for switching between operating systems (Windows, Linux, macOS).
- [ ] Build dynamic `TableOfContents` with active heading scrollspy.
- [ ] Build `PrevNextNav` for smooth documentation page sequential navigation.

### Phase D3: Global Search System (`Cmd+K`)
- [ ] Generate comprehensive JSON search index mapping all topics, sections, and stdlib APIs.
- [ ] Implement `SearchModal` component activated by click or `Cmd+K` / `Ctrl+K`.
- [ ] Implement fast fuzzy search filtering with title, excerpt, and badge category tags.
- [ ] Support keyboard navigation (Up, Down, Enter, Esc) with instant routing.

### Phase D4: Comprehensive Documentation Pages
- [ ] Write Getting Started pages: Installation, First Program, Project Setup.
- [ ] Write Language Tour pages covering all language features from [`SP-Syntax-Guide.md`](file:///k:/AndroidDev/tools/Project/Own%20Programming%20Language/SP-Language/SP-Syntax-Guide.md).
- [ ] Write Standard Library API reference pages for all 8 core modules.
- [ ] Write Developer Tools guides for `spc`, `spm`, `spls`, `spd`, and VS Code Extension.
- [ ] Write Benchmarks page with interactive tables and charts.

### Phase D5: Optimization, SEO & Production Deployment
- [ ] Add OpenGraph metadata, Favicon, robots.txt, and sitemap.xml.
- [ ] Configure custom domain routing for `docs.splang.shantopaul.com`.
- [ ] Add automated build scripts (`npm run build` / `npm run export`).
- [ ] Verify responsive layout across mobile, tablet, and widescreen monitors.

---

## 6. Success Criteria & Deliverables
- Fully operational Next.js website running inside `/splang-docs`.
- Instant search (`Ctrl+K`) returns precise results for any syntax feature or standard library symbol.
- Every standard library module and language construct has complete explanations and syntax-highlighted code.
- Clean production build (`npm run build`) with zero linting or TypeScript errors.
