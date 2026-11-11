# ToDoList — Production Fullstack Application (SP Language v4.0.0 + Next.js 14)

A multi-user, production-grade ToDoList workspace featuring a **100% pure SP Programming Language v4.0.0** backend API and a **Next.js 14 + TypeScript + Tailwind CSS** web client.

---

## Tech Stack

### Backend (`examples/ToDoList/backend/`)
- **Language & Compiler:** Pure **SP Programming Language v4.0.0** (`spc`) compiled natively via LLVM
- **HTTP & Routing:** `std.web` (`App`, `Context`, CORS, Request ID, Recovery, Logger, Rate Limiter, Swagger/ReDoc) & `std.http`
- **Database:** Embedded **SQLite** (`std.db`) in **WAL mode** with `PRAGMA foreign_keys = ON` and strictly parameterized `SqliteStatement` queries
- **Security:** **Argon2id** password hashing (`std.crypto.password`), **HMAC-SHA256 JWT** (`std.security.jwt`), fail-closed `JWT_SECRET` (>= 32 bytes), and strict per-user tenant ownership guards
- **API Errors:** **RFC 7807** `application/problem+json` with field-level validation issues

### Frontend (`examples/ToDoList/frontend/`)
- **Framework:** **Next.js 14 (App Router)** + **TypeScript (`strict: true`)**
- **Styling & Icons:** **Tailwind CSS** (light & dark themes) + **Lucide Icons**
- **UX Features:**
  - JWT auth flows (`/login`, `/register`, persistent hydration, auto sign-out on 401)
  - Workspace views (`All Tasks`, `Due Today`, `Overdue`, `In Progress`, `Completed`, `Archived`, per-category counts, `Uncategorized`)
  - Clickable KPI stats cards, quick-add bar (`n` keyboard shortcut), debounced search (`/` shortcut), slide-over detail drawer (`Escape` closes), optimistic completion toggles with automatic rollback, transactional bulk actions, category color customization, and `/settings` profile + backend health diagnostics

---

## Directory Layout

```text
examples/ToDoList/
├── backend/                  # 100% pure SP Language backend
│   ├── sp.toml
│   ├── openapi.json
│   ├── .env.example
│   ├── src/
│   │   ├── config/env.sp
│   │   ├── db/schema.sp
│   │   ├── models/{user,category,todo}.sp
│   │   ├── repositories/{user_repo,category_repo,todo_repo}.sp
│   │   ├── services/{auth_service,category_service,todo_service}.sp
│   │   ├── middleware/auth.sp
│   │   ├── controllers/{health,auth,category,todo}_controller.sp
│   │   ├── routes/router.sp
│   │   ├── util/{json,validation,error,logger}.sp
│   │   └── main.sp
│   └── tests/{auth_test,category_test,todo_test}.sp
├── frontend/                 # Next.js 14 App Router client
│   ├── .env.example
│   ├── package.json
│   └── src/
│       ├── app/{layout,page,login/page,register/page,settings/page}.tsx
│       ├── components/{ui,layout,auth,categories,todos}/
│       ├── hooks/{useTodos,useCategories,useTheme,useKeyboardShortcuts}.ts
│       ├── lib/{api-client,auth-context,toast-context,config,storage,date,cn}.ts
│       └── types/{api,auth,category,todo}.ts
├── docs/
│   ├── ARCHITECTURE.md
│   ├── API_CONTRACT.md
│   └── DATABASE_SCHEMA.md
├── run_backend.ps1
├── run_backend_tests.ps1
└── run_frontend.ps1
```

---

## Quickstart

### Prerequisites
- **SP Language v4.0.0** (`spc` available on `PATH`)
- **Node.js 18+** & **npm**

### 1. Start the SP Backend (`http://localhost:8080`)

Copy `backend/.env.example` to `backend/.env` and set a strong `JWT_SECRET` (at least 32 characters):

```powershell
Copy-Item examples/ToDoList/backend/.env.example examples/ToDoList/backend/.env
```

Or run the convenience script (builds `build/todolist-backend.exe` and starts it):

```powershell
.\examples\ToDoList\run_backend.ps1
```

Interactive API documentation once running:
- Swagger UI: `http://localhost:8080/docs`
- ReDoc: `http://localhost:8080/redoc`
- OpenAPI Spec: `http://localhost:8080/openapi.json`
- Health Probe: `http://localhost:8080/api/health`

### 2. Start the Next.js Frontend (`http://localhost:3000`)

```powershell
.\examples\ToDoList\run_frontend.ps1
```

Or manually:

```powershell
cd examples/ToDoList/frontend
Copy-Item .env.example .env.local
npm install
npm run dev
```

---

## Running Verification Suites

### Backend Integration Tests (Compiled Pure SP Suites)

```powershell
.\examples\ToDoList\run_backend_tests.ps1
```

This compiles and executes:
1. `tests/auth_test.sp` — Live HTTP server (`app.listen`), health/readiness probes, registration, duplicate email `409`, Argon2id password checks, JWT verification, and profile updates
2. `tests/category_test.sp` — Default category seeding, CRUD, case-insensitive uniqueness, `todo_count` aggregation, `ON DELETE SET NULL`, and cross-tenant `403` isolation
3. `tests/todo_test.sp` — Full CRUD, `completed_at` status transitions, nullable field clears, search/filter/sort/pagination, `stats` aggregation, transactional `bulk` actions, and strict multi-tenant `403` isolation

### Frontend Quality Checks

```powershell
cd examples/ToDoList/frontend
npx tsc --noEmit
npm run lint
npm run build
```

---

## Further Documentation

- [Architecture Guide](./docs/ARCHITECTURE.md)
- [REST API Contract](./docs/API_CONTRACT.md)
- [SQLite Database Schema](./docs/DATABASE_SCHEMA.md)
