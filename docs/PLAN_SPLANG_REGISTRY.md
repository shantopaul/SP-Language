# Implementation Plan: Official Package Registry Portal (`registry.splang.shantopaul.com`)

## 1. Executive Summary & Objective

The **Official SP Package Registry** (`https://registry.splang.shantopaul.com`) is the central repository and discovery platform for open-source libraries, frameworks, and developer tools in the SP ecosystem. Modeled after premier language registries like `crates.io`, `npmjs.com`, and `pkg.go.dev`, it provides:
1. An intuitive, fast web portal for searching, inspecting, and documenting SP packages.
2. Production-ready JSON API endpoints serving the SP Package Manager (`spm`) for commands such as `spm search`, `spm install`, `spm publish`, and `spm verify`.
3. Cryptographic integrity verification via SHA-256 checksums to ensure tamper-proof package distribution.

The project will reside in the root repository folder under `/splang-registry` and will be engineered using **Next.js 14+ (App Router)**, **TypeScript**, **Tailwind CSS**, and modern UI components.

---

## 2. Technical Stack & Architecture

- **Framework:** Next.js 14+ (App Router with Server Components and API Route Handlers)
- **Language:** TypeScript (strict mode)
- **Styling:** Tailwind CSS with responsive layout and dark/light mode
- **Icons:** `lucide-react`
- **Data Layer:** Structured in-memory / JSON file data store with full CRUD capabilities and initial pre-seeded ecosystem packages; ready for MongoDB, PostgreSQL, or SQLite integration
- **API Endpoints:** RESTful JSON API under `/api/v1/*` conforming to the `spm` package manager protocol specification
- **Security & Integrity:** SHA-256 cryptographic verification matching `sp.lock` requirements
- **Target Domain:** `registry.splang.shantopaul.com`

---

## 3. Directory Structure (`/splang-registry`)

```text
splang-registry/
├── public/
│   ├── favicon.ico
│   ├── logo.svg
│   └── og-registry.png
├── src/
│   ├── app/
│   │   ├── globals.css
│   │   ├── layout.tsx                     # Root layout with search header, theme provider, and footer
│   │   ├── page.tsx                       # Registry home (Search hero, popular packages, recently published)
│   │   ├── packages/
│   │   │   ├── page.tsx                   # Full packages catalog with category filters and sorting
│   │   │   └── [name]/
│   │   │       └── page.tsx               # Individual package detail page (Readme, Versions, Dependencies, Security)
│   │   ├── publish/
│   │   │   └── page.tsx                   # Publisher guide and authentication walkthrough
│   │   ├── categories/
│   │   │   └── [category]/page.tsx        # Filter packages by category (cli, web, math, db, etc.)
│   │   ├── security/
│   │   │   └── page.tsx                   # Security audit policy, lockfile verification, and integrity checks
│   │   └── api/
│   │       └── v1/
│   │           ├── packages/
│   │           │   ├── route.ts           # GET: List packages with pagination and sorting
│   │           │   └── [name]/
│   │           │       └── route.ts       # GET: Package metadata, versions, and checksums
│   │           ├── search/
│   │           │   └── route.ts           # GET: Search query endpoint for spm search
│   │           ├── publish/
│   │           │   └── route.ts           # POST: Package publication endpoint for spm publish
│   │           └── verify/
│   │               └── [name]/
│   │                   └── [version]/
│   │                       └── route.ts   # GET: Integrity hash verification for spm verify
│   ├── components/
│   │   ├── layout/
│   │   │   ├── Navbar.tsx                 # Navigation with quick search, links, and publish button
│   │   │   └── Footer.tsx                 # Registry footer with documentation and status links
│   │   ├── registry/
│   │   │   ├── PackageCard.tsx            # Compact package card for grid/list views
│   │   │   ├── PackageHeader.tsx          # Large header with stats (version, downloads, license, author)
│   │   │   ├── InstallCommand.tsx         # Copyable `spm add <pkg>` command widget
│   │   │   ├── ReadmeViewer.tsx           # Markdown renderer for package README.md
│   │   │   ├── VersionHistory.tsx         # Release history list with timestamps and changelogs
│   │   │   ├── DependencyTree.tsx         # Visual representation of package dependencies
│   │   │   ├── SecurityBadge.tsx          # SHA-256 checksum card and audit indicator
│   │   │   └── CategoryFilter.tsx         # Category pills for filtering
│   │   └── ui/
│   │       ├── SearchInput.tsx            # Search bar with debounced input and clear button
│   │       ├── Badge.tsx                  # Tag pills for keywords and categories
│   │       ├── Tabs.tsx                   # Detail page tab switcher
│   │       └── CopyButton.tsx             # One-click clipboard copy with tooltip
│   ├── data/
│   │   ├── packages-db.ts                 # Pre-seeded packages database and helper querying functions
│   │   └── categories.ts                  # Registered ecosystem categories and descriptions
│   ├── types/
│   │   └── package.ts                     # TypeScript definitions for Package, Version, Author, Dependency
│   └── lib/
│       ├── registry-api.ts                # Client API helpers
│       └── utils.ts                       # Formatters for download counts, dates, and byte sizes
├── next.config.mjs
├── package.json
├── postcss.config.mjs
├── tailwind.config.ts
└── tsconfig.json
```

---

## 4. Pre-Seeded Ecosystem Packages

The registry will launch with 8 high-quality, pre-seeded standard community packages:

1. **`json` (v1.2.0)**
   - *Description:* High-performance, streaming JSON parser and serializer with native SP type mapping.
   - *Author:* SP Core Team (`core@splang.shantopaul.com`)
   - *Category:* Utilities / Web
   - *Downloads:* 142,500

2. **`http` (v1.1.0)**
   - *Description:* Asynchronous HTTP client and server library with middleware routing.
   - *Author:* SP Core Team
   - *Category:* Web / Networking
   - *Downloads:* 98,200

3. **`crypto` (v1.0.4)**
   - *Description:* Cryptographic primitives including SHA-256, HMAC, AES-256-GCM, and secure random byte generation.
   - *Author:* Security Working Group
   - *Category:* Security / Cryptography
   - *Downloads:* 74,100

4. **`cli` (v1.3.1)**
   - *Description:* Robust command-line argument parser with subcommands, automatic help generation, and interactive prompts.
   - *Author:* Developer Tools WG
   - *Category:* CLI / Tools
   - *Downloads:* 112,800

5. **`log` (v1.0.2)**
   - *Description:* Fast, structured logging library supporting JSON formatting, level filtering, and colored terminal outputs.
   - *Author:* SP Core Team
   - *Category:* Utilities / Logging
   - *Downloads:* 89,600

6. **`math-matrix` (v0.9.5)**
   - *Description:* Linear algebra and matrix mathematics library optimized for scientific computing.
   - *Author:* Scientific SP Team
   - *Category:* Math / Science
   - *Downloads:* 31,400

7. **`db-sqlite` (v1.0.1)**
   - *Description:* Embedded SQLite database driver with prepared statements and transaction safety.
   - *Author:* Database WG
   - *Category:* Database / Storage
   - *Downloads:* 56,200

8. **`test-bench` (v1.0.0)**
   - *Description:* Micro-benchmarking and test assertions library with statistical runtime variance reporting.
   - *Author:* Developer Tools WG
   - *Category:* Development / Testing
   - *Downloads:* 45,900

---

## 5. API Endpoints Specification for `spm` Integration

The Next.js Route Handlers (`src/app/api/v1/`) implement the official registry protocol:

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/v1/packages` | List packages with query filters (`page`, `limit`, `sort`, `category`). |
| `GET` | `/api/v1/packages/:name` | Fetch package metadata, all versions, author info, and dependencies. |
| `GET` | `/api/v1/search?q=:query` | Full-text package search for CLI (`spm search <query>`). |
| `POST` | `/api/v1/publish` | Upload and publish a package tarball with `sp.toml` manifest. |
| `GET` | `/api/v1/verify/:name/:version` | Return the authoritative SHA-256 checksum for lockfile verification (`spm verify`). |

---

## 6. Implementation Phases for `/splang-registry`

### Phase R1: Project Scaffolding & Data Modeling
- [ ] Initialize `/splang-registry` directory with Next.js 14+ App Router, TypeScript, and Tailwind CSS.
- [ ] Define TypeScript interfaces for `PackageMetadata`, `VersionInfo`, `Dependency`, and `RegistryResponse`.
- [ ] Implement data store in `packages-db.ts` populated with the 8 official packages.
- [ ] Setup base layout with responsive navigation, quick search header, and footer.

### Phase R2: Package Catalog & Search Experience
- [ ] Build Registry homepage featuring search hero, category pills, total packages counter, and download metrics.
- [ ] Implement packages catalog (`/packages`) with real-time search, sorting (Most Downloaded, Newest, Name), and category filtering.
- [ ] Implement category detail views (`/categories/[category]`).

### Phase R3: Package Detail & Multi-Tab Viewer
- [ ] Build `/packages/[name]` detail page displaying package metadata, badges, and copyable `spm add <pkg>` command.
- [ ] Implement `ReadmeViewer` with formatted markdown rendering and code syntax highlighting.
- [ ] Implement `VersionHistory` tab listing published releases with timestamps and download links.
- [ ] Implement `DependencyTree` tab showing direct and transitive dependencies.
- [ ] Implement `SecurityBadge` tab displaying the cryptographic SHA-256 hash and audit status.

### Phase R4: RESTful API Route Handlers for `spm` CLI
- [ ] Implement `GET /api/v1/packages` route handler.
- [ ] Implement `GET /api/v1/packages/[name]` route handler.
- [ ] Implement `GET /api/v1/search` route handler matching `spm search` queries.
- [ ] Implement `POST /api/v1/publish` route handler with validation for `spm publish`.
- [ ] Implement `GET /api/v1/verify/[name]/[version]` returning cryptographic checksums for `spm verify`.

### Phase R5: Publisher Guide & Production Readiness
- [ ] Implement `/publish` guide explaining how to write `sp.toml`, test locally, and run `spm publish`.
- [ ] Implement `/security` audit policy and tamper detection guidelines.
- [ ] Configure custom domain headers and deployment config for `registry.splang.shantopaul.com`.
- [ ] Validate responsive design and verify clean production build (`npm run build`).

---

## 7. Success Criteria & Deliverables
- Fully operational Next.js website running inside `/splang-registry`.
- Complete web UI allowing developers to search, browse, filter, and inspect package documentation and version history.
- Functional JSON REST API under `/api/v1/*` responding accurately to requests from `spm` CLI commands.
- Cryptographic SHA-256 verification displayed for every package release.
- Clean production build (`npm run build`) with zero linting or TypeScript errors.
