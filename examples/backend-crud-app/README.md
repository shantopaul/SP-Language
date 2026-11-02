# Fullstack CRUD Application: SP Lang 3.0 Backend & Next.js 14 Frontend

Production-grade fullstack application showcasing **SP Lang 3.0** as an ultra-high performance native backend language connected to **Neon Cloud PostgreSQL** and serving modern frontend applications (Next.js 14 App Router with Tailwind CSS).

---

## 🏗️ Architecture & Modular Project Structure

The SP Lang backend (`backend-splang`) is built following modern, production-grade clean architecture separated into modular domain layers:

```text
examples/backend-crud-app/
├── backend-splang/                      # Native SP Lang 3.0 REST Backend
│   ├── .env                             # Environment Variables & Secrets
│   ├── .env.example                     # Reference Environment Template
│   ├── package.json                     # NPM Scripts for dev / build / start
│   ├── server.exe                       # Compiled Native High-Speed Binary
│   └── src/
│       ├── config/
│       │   └── env.sp                   # Env class: loads .env & OS env vars
│       ├── database/
│       │   └── connection.sp            # DatabaseManager: Neon Cloud PG & fallback
│       ├── models/
│       │   └── item.sp                  # ItemModel & JSON sanitization
│       ├── middleware/
│       │   └── auth.sp                  # AuthMiddleware: API Key security guard
│       ├── controllers/
│       │   ├── health_controller.sp     # Health probe & OpenAPI spec handler
│       │   └── item_controller.sp       # REST CRUD actions with auth check
│       ├── routes/
│       │   └── router.sp                # AppRouter: maps HTTP endpoints to handlers
│       └── main.sp                      # High-level entrypoint & server bootstrap
│
└── fontend-next/                        # Next.js 14 App Router Frontend
    ├── .env.local                       # NEXT_PUBLIC_API_URL & NEXT_PUBLIC_API_KEY
    ├── package.json                     # Next.js dependencies & scripts
    └── src/
        ├── app/                         # App Router layout & interactive dashboard
        └── lib/
            └── api.ts                   # Robust API client passing X-API-Key
```

---

## 🔐 Environment Configuration & API Key Authentication

### Backend `.env` (`backend-splang/.env`):
```env
# HTTP Server Configuration
PORT=8000
APP_NAME=SP Lang Fullstack API
APP_ENV=production

# Neon Cloud PostgreSQL Connection URL (TLS/SSL Wire Protocol)
DATABASE_URL=postgresql://user:password@ep-pooler.aws.neon.tech/neondb?sslmode=require&channel_binding=require

# Secure API Key for Authenticated Endpoints
API_KEY=sp_live_your_secret_api_key_here
```

### Frontend `.env.local` (`fontend-next/.env.local`):
```env
NEXT_PUBLIC_API_URL=http://localhost:8000
NEXT_PUBLIC_API_KEY=sp_live_your_secret_api_key_here
```

### Authentication Rules:
- **Public Endpoints**: `GET /api/health`, `GET /api/items`, `GET /api/items/:id`, `GET /openapi.json`, `GET /docs`, `GET /redoc`.
- **Protected Mutations**: `POST /api/items`, `PUT /api/items/:id`, `DELETE /api/items/:id` require valid credentials via:
  - Header: `X-API-Key: sp_live_your_secret_api_key_here`
  - Or Header: `Authorization: Bearer sp_live_your_secret_api_key_here`
  - Missing/invalid key results in `401 Unauthorized`.

---

## 🚀 Quick Start Commands

### 1. Run Backend on `http://localhost:8000`

```bash
# Terminal 1: Navigate to backend-splang
cd examples/backend-crud-app/backend-splang

# Start development server (auto-compiles and runs server.exe):
npm run dev
```

The SP Lang native binary will bootstrap:
```text
==========================================================================
           SP Lang 3.0 Modular Backend Service (backend-splang)           
==========================================================================
[✓] Loaded environment config. Port: 8000, App: SP Lang Fullstack API
[+] Initializing database schemas...
[✓] Connected to Neon Cloud PostgreSQL (AWS US-East-2)
[✓] Database initialized and verified.

[+] SP Lang backend server live at http://localhost:8000
    * REST API Root:   http://localhost:8000/api/items
    * Health Probe:    http://localhost:8000/api/health
    * Swagger UI Docs: http://localhost:8000/docs
    * Redoc View:      http://localhost:8000/redoc
    * Security:        API Key authentication enabled on write endpoints
==========================================================================
```

### 2. Run Frontend on `http://localhost:3000`

```bash
# Terminal 2: Navigate to fontend-next
cd examples/backend-crud-app/fontend-next

# Start Next.js development server:
npm run dev
```

Visit **`http://localhost:3000`** in your browser to interact with the fullstack dashboard.

---

## 📡 REST API Reference (Port 8000)

| Method | Endpoint | Auth Required | Description | Status Code |
|---|---|:---:|---|---|
| `GET` | `/api/health` | No | Neon Cloud PostgreSQL connectivity, stats, and uptime | `200 OK` |
| `GET` | `/api/items` | No | Retrieve all items from Neon PostgreSQL (ordered descending) | `200 OK` |
| `GET` | `/api/items/:id` | No | Retrieve single item by ID | `200 OK` / `404 Not Found` |
| `POST` | `/api/items` | **Yes (`X-API-Key`)** | Create new item (`title`, `description`, `status`, `priority`) | `201 Created` / `401 Unauthorized` |
| `PUT` | `/api/items/:id` | **Yes (`X-API-Key`)** | Update existing item fields | `200 OK` / `401 Unauthorized` / `404 Not Found` |
| `DELETE` | `/api/items/:id` | **Yes (`X-API-Key`)** | Delete item by ID from Neon Cloud | `200 OK` / `401 Unauthorized` / `404 Not Found` |
| `GET` | `/openapi.json` | No | OpenAPI 3.0.3 specification JSON | `200 OK` |
| `GET` | `/docs` | No | Embedded Interactive Swagger UI viewer | `200 OK` |
| `GET` | `/redoc` | No | Embedded Interactive Redoc API documentation | `200 OK` |

---

## 🐘 Neon Cloud PostgreSQL Database

Connected via **PostgreSQL Wire Protocol 3.0 with native OpenSSL TLS & SNI host negotiation**:
```text
postgresql://user:password@ep-pooler.aws.neon.tech/neondb?sslmode=require&channel_binding=require
```

### Table Schema:
```sql
CREATE TABLE IF NOT EXISTS items (
    id SERIAL PRIMARY KEY,
    title TEXT NOT NULL,
    description TEXT,
    status TEXT DEFAULT 'pending',
    priority TEXT DEFAULT 'normal',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```
