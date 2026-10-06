# Implementation Plan: Official Main Website (`splang.shantopaul.com`)

## 1. Executive Summary & Objective

The **Official SP Language Landing Website** (`https://splang.shantopaul.com`) is the flagship online presence for the SP Programming Language. Its mission is to introduce the language to the global developer community, articulate its value proposition, demonstrate language capabilities through interactive code showcases, present verifiable benchmarks, and direct developers to the documentation and package registry.

The project will reside in the root repository folder under `/splang` and will be engineered using **Next.js 14+ (App Router)**, **TypeScript**, **Tailwind CSS**, and modern visual animations.

---

## 2. Technical Stack & Architecture

- **Framework:** Next.js 14+ (App Router with SSR and Static Generation)
- **Language:** TypeScript (strict mode)
- **Styling:** Tailwind CSS with custom design tokens (dark/light themes, modern tech neon accents, glassmorphic UI)
- **Icons:** `lucide-react`
- **Animations:** Tailwind CSS transitions and smooth micro-interactions
- **Component Primitives:** Accessible, headless UI patterns for tabs, copy buttons, modals, and tooltips
- **SEO & Social Sharing:** Dynamic OpenGraph preview images, Twitter cards, JSON-LD structured schema for SoftwareApplication
- **Target Domain:** `splang.shantopaul.com`

---

## 3. Directory Structure (`/splang`)

```text
splang/
├── public/
│   ├── favicon.ico
│   ├── logo.svg
│   ├── hero-banner.png
│   ├── og-main.png
│   ├── vscode-preview.png
│   └── install.sh                         # Web-accessible curl installer script
├── src/
│   ├── app/
│   │   ├── globals.css
│   │   ├── layout.tsx                     # Root layout with site navbar, theme toggle, and footer
│   │   ├── page.tsx                       # Homepage (Hero, Features, Code Showcase, Benchmarks, Install)
│   │   ├── downloads/
│   │   │   └── page.tsx                   # Direct platform binary downloads (Windows, Linux, macOS)
│   │   ├── benchmarks/
│   │   │   └── page.tsx                   # Interactive benchmarks explorer vs C, C++, Rust, Java, Node, Python
│   │   ├── blog/
│   │   │   ├── page.tsx                   # Blog index (Release notes, engineering articles)
│   │   │   └── [slug]/page.tsx            # Blog post reader (v1.0.0 Release announcement)
│   │   ├── community/
│   │   │   └── page.tsx                   # Contributing guidelines, Discord/Forum/GitHub links
│   │   └── not-found.tsx                  # 404 handler
│   ├── components/
│   │   ├── layout/
│   │   │   ├── Navbar.tsx                 # Sticky navigation header with domain links and GitHub counter
│   │   │   └── Footer.tsx                 # Comprehensive ecosystem footer
│   │   ├── sections/
│   │   │   ├── HeroSection.tsx            # Main headline, value prop, CTA buttons, and quick install box
│   │   │   ├── QuickInstall.tsx           # Multi-platform tabbed install snippet (PowerShell, Curl, Winget)
│   │   │   ├── CodeShowcase.tsx           # Interactive tabbed code viewer highlighting SP syntax idioms
│   │   │   ├── KeyFeatures.tsx            # Grid of language pillars (Performance, Safety, Tooling)
│   │   │   ├── BenchmarkVisualizer.tsx    # Interactive bar charts for 7-language performance data
│   │   │   ├── ToolingShowcase.tsx        # Features spotlight: spc fmt, test, doc, spls, and VS Code
│   │   │   ├── EcosystemGrid.tsx          # Direct links to Docs, Registry, GitHub, and Packages
│   │   │   └── ReleaseCallout.tsx         # SP 1.0.0 announcement highlight banner
│   │   └── ui/
│   │       ├── Button.tsx                 # Primary, secondary, outline button variants
│   │       ├── CodeHighlighter.tsx        # Client syntax highlighter for SP code
│   │       ├── Badge.tsx                  # Version and status pills
│   │       └── Card.tsx                   # Feature and testimonial cards
│   ├── data/
│   │   ├── benchmarks-data.ts             # Structured benchmark runtime and memory numbers
│   │   ├── code-snippets.ts               # Curated code examples (Basics, OOP, Generics, Lambdas)
│   │   └── releases.ts                    # GitHub release metadata and download links
│   └── lib/
│       ├── utils.ts                       # Classnames merger and helper utilities
│       └── metadata.ts                    # SEO and OpenGraph generator
├── next.config.mjs
├── package.json
├── postcss.config.mjs
├── tailwind.config.ts
└── tsconfig.json
```

---

## 4. Key Sections & Functional Requirements

### 1. Hero Section & Quick Install
- **Headline:** *"The Modern, High-Performance Systems & Application Language"*
- **Subheadline:** *"SP combines the raw execution speed and zero-cost abstractions of LLVM native compilation with the modern developer ergonomics of safe null navigation, expressive pattern matching, and built-in package management."*
- **Action Buttons:**
  - Primary: `Get Started ->` (redirects to `https://docs.splang.shantopaul.com/getting-started`)
  - Secondary: `Explore Documentation` (redirects to `https://docs.splang.shantopaul.com`)
  - Outline: `GitHub Star` (links to `https://github.com/shantopaul/SP-Language`)
- **Quick Install Box:** Tabbed selector for:
  - **Windows:** `winget install SPLang.SP` or `iwr -useb https://splang.shantopaul.com/install.ps1 | iex`
  - **Linux / macOS:** `curl -fsSL https://splang.shantopaul.com/install.sh | sh`
  - **Docker:** `docker pull shantopaul/splang:1.0.0`

### 2. Interactive Code Showcase
Tabbed code viewer demonstrating elegance, type safety, and syntax versatility:
- **Hello World & Basics:** Variable inference, string interpolation, and console I/O.
- **Pattern Matching:** Exhaustive `match` expressions with destructuring.
- **Object-Oriented Design:** Classes, constructors, interfaces, and polymorphic dispatch.
- **Generics & Collections:** `List<T>`, `Map<K, V>`, and functional chained iterators (`map`, `filter`).
- **Standard Library:** Native filesystem operations and process spawning.

### 3. Interactive Benchmark Visualizer
Direct visual comparison of SP vs 6 other industry languages based on [`docs/BENCHMARKS.md`](file:///k:/AndroidDev/tools/Project/Own%20Programming%20Language/SP-Language/docs/BENCHMARKS.md):
- Workloads: **Recursive Fibonacci (n=40)**, **Quicksort (1M items)**, **Hash Map (1M ops)**, **Binary Trees (depth 16)**, **Mandelbrot (1000x1000)**.
- Languages: **SP**, **C (GCC -O3)**, **C++ (Clang -O3)**, **Rust (Cargo release)**, **Java (OpenJDK 21)**, **Node.js (v20)**, **Python (3.12)**.
- Metrics: Execution Time (ms) and Peak Resident Memory (MB) with interactive workload switcher.

### 4. Built-In Developer Tooling Spotlight
- **`spc fmt`:** Zero-configuration, idempotent code formatter.
- **`spc test`:** Native test runner parsing `test "name" { ... }` blocks with pass/fail metrics.
- **`spc doc`:** Automatic HTML & Markdown documentation generator from `///` docstrings.
- **`spls` & VS Code Extension:** Real-time diagnostics, autocomplete, hover docstrings, and DAP debugging.
- **`spm`:** Next-generation package manager with deterministic `sp.lock` cryptographic verification.

### 5. Official Ecosystem Navigation
Direct cross-links to the companion portals:
- **Documentation:** `https://docs.splang.shantopaul.com`
- **Package Registry:** `https://registry.splang.shantopaul.com`
- **GitHub Repository:** `https://github.com/shantopaul/SP-Language`

---

## 5. Implementation Phases for `/splang`

### Phase M1: Scaffolding & Design Foundation
- [ ] Initialize `/splang` directory with Next.js 14+ App Router, TypeScript, and Tailwind CSS.
- [ ] Configure custom theme tokens, typography, dark/light theme styles, and animations.
- [ ] Implement responsive layout, sticky navbar, and ecosystem footer.

### Phase M2: Hero Section & Quick Install Component
- [ ] Build high-impact `HeroSection` with copyable quick-install terminal widget.
- [ ] Build tabbed installer supporting Windows (PowerShell/Winget), Linux (Bash), macOS, and Docker.
- [ ] Add version badges and release tags (v1.0.0).

### Phase M3: Interactive Code Showcase & Benchmarks
- [ ] Build `CodeShowcase` component with interactive tab switching between idiomatic SP code examples.
- [ ] Build `BenchmarkVisualizer` with interactive charts rendering performance comparisons against C, C++, Rust, Java, Node.js, and Python.
- [ ] Include detailed notes on LLVM optimization levels (`-O3`) and methodology.

### Phase M4: Toolchain & Downloads Pages
- [ ] Build `ToolingShowcase` highlighting `spc`, `spm`, `spls`, and the VS Code extension.
- [ ] Implement dedicated `/downloads` page with direct platform asset links (`.zip`, `.tar.gz`, SHA-256 checksums).
- [ ] Implement `/blog` section with the official v1.0.0 launch announcement post.

### Phase M5: Production Optimization & Deployment
- [ ] Add OpenGraph social preview images, sitemap, and robots.txt.
- [ ] Validate responsive design across all viewports (Mobile, Tablet, Desktop).
- [ ] Configure domain routing for `splang.shantopaul.com`.
- [ ] Verify zero build warnings and 100% lighthouse audit readiness.

---

## 6. Success Criteria & Deliverables
- Fully operational Next.js website running inside `/splang`.
- Engaging, professional hero section with working install scripts and copy functionality.
- Interactive benchmark visualizer accurately displaying the 7-language data from [`docs/BENCHMARKS.md`](file:///k:/AndroidDev/tools/Project/Own%20Programming%20Language/SP-Language/docs/BENCHMARKS.md).
- Clean production build (`npm run build`) with zero errors.
