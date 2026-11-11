# ToDoList REST API Contract

**Base URL:** `http://localhost:8080`  
**OpenAPI Spec:** `GET /openapi.json`  
**Interactive Docs:** `GET /docs` (Swagger UI) · `GET /redoc` (ReDoc)

---

## Error Format (RFC 7807 Problem Details)

All `4xx` / `5xx` API errors return `Content-Type: application/problem+json; charset=utf-8`:

```json
{
  "type": "https://todolist.splang.dev/errors/validation",
  "title": "Validation Failed",
  "status": 422,
  "detail": "Todo input failed validation.",
  "instance": "/api/v1/todos",
  "request_id": "req-...",
  "errors": [
    {
      "field": "title",
      "code": "INVALID_LENGTH",
      "message": "Title must be between 1 and 200 characters."
    }
  ]
}
```

| HTTP Status | Meaning |
| :--- | :--- |
| `400` | Malformed JSON body or invalid UUID path parameter |
| `401` | Missing/expired/invalid Bearer JWT or bad login credentials |
| `403` | Authenticated user attempted to access another user's resource |
| `404` | Category or Todo ID does not exist |
| `409` | Email already registered or duplicate category name for user |
| `422` | Field-level schema or enum validation error (`errors[]` populated) |
| `429` | Rate limit exceeded |
| `500` | Internal server/database failure |

---

## Health Endpoints

| Method | Path | Auth | Response |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/health` | Public | `200` `{"status":"healthy","service":"todolist-backend","version":"1.0.0","database":"up"}` (`503` if DB unreachable) |
| `GET` | `/healthz` | Public | `200` liveness JSON |
| `GET` | `/readyz` | Public | `200` readiness JSON (`503` if DB unreachable) |

---

## Authentication & Profile

### `POST /api/v1/auth/register`
Creates a user account with Argon2id password hash, seeds default categories (`Work`, `Personal`, `Errands`), and issues a JWT.

**Request:**
```json
{
  "name": "Alice Johnson",
  "email": "alice@example.com",
  "password": "StrongPassword123"
}
```
**Response (`201 Created`):**
```json
{
  "access_token": "<jwt>",
  "token_type": "Bearer",
  "expires_in": 86400,
  "user": {
    "id": "uuid",
    "email": "alice@example.com",
    "name": "Alice Johnson",
    "created_at": "2026-11-11T08:00:00Z",
    "updated_at": "2026-11-11T08:00:00Z"
  }
}
```

### `POST /api/v1/auth/login`
Authenticates email + password and issues a Bearer JWT (`200 OK`).

### `GET /api/v1/auth/me` *(Bearer Required)*
Returns `{"user": { ... }}` (`200 OK`).

### `PATCH /api/v1/auth/me` *(Bearer Required)*
Updates authenticated user display `name` (`2..80` chars) and returns `{"user": { ... }}`.

---

## Categories *(Bearer Required)*

### `GET /api/v1/categories`
Lists categories owned by the authenticated user sorted by `(sort_order ASC, name ASC)` including live `todo_count`.

### `POST /api/v1/categories`
**Request:**
```json
{
  "name": "Engineering",
  "color": "#3B82F6",
  "sort_order": 4
}
```
- `name`: 1–40 chars, case-insensitively unique per user (`409 Conflict` on duplicate).
- `color`: `#RRGGBB` 6-digit hex (defaults to `#3B82F6`).

### `PATCH /api/v1/categories/:id` & `PUT /api/v1/categories/:id`
Partially updates `name`, `color`, or `sort_order`. Returns `403` if owned by another user.

### `DELETE /api/v1/categories/:id`
Deletes category (`200 OK`, `{"deleted": true, "id": "..."}`). Associated todos are kept with `category_id = null`.

---

## Todos *(Bearer Required)*

### `GET /api/v1/todos`
Lists paginated tasks belonging strictly to the authenticated user.

| Query Param | Values | Default |
| :--- | :--- | :--- |
| `status` | `all`, `active`, `todo`, `in_progress`, `done`, `archived` | `all` |
| `priority` | `all`, `low`, `medium`, `high`, `urgent` | `all` |
| `category_id` | `all`, `uncategorized`, `<uuid>` | `all` |
| `due` | `all`, `overdue`, `today`, `upcoming`, `none` | `all` |
| `q` (or `search`) | Substring matched across `title` and `description` | empty |
| `sort` | `created_at`, `due_date`, `priority`, `updated_at`, `title`, `sort_order` | `created_at` |
| `order` | `asc`, `desc` | `desc` |
| `page` | Integer >= 1 | `1` |
| `limit` | Integer 1–100 | `20` |

**Response (`200 OK`):**
```json
{
  "items": [
    {
      "id": "uuid",
      "user_id": "uuid",
      "category_id": "uuid-or-null",
      "category": { "id": "uuid", "name": "Work", "color": "#3B82F6" },
      "title": "Ship release notes",
      "description": "Coordinate docs & screenshots",
      "status": "todo",
      "priority": "high",
      "due_date": "2026-11-20",
      "completed_at": null,
      "sort_order": 0,
      "created_at": "2026-11-11T08:15:00Z",
      "updated_at": "2026-11-11T08:15:00Z"
    }
  ],
  "meta": {
    "total": 1,
    "page": 1,
    "limit": 20,
    "total_pages": 1,
    "has_next": false,
    "has_prev": false
  }
}
```

### `GET /api/v1/todos/stats`
Returns aggregated workspace counters for sidebar & dashboard KPI cards:
```json
{
  "total": 12,
  "todo": 4,
  "in_progress": 3,
  "done": 4,
  "archived": 1,
  "overdue": 1,
  "due_today": 2,
  "urgent": 2,
  "high": 3
}
```

### `POST /api/v1/todos`
Creates a todo (`201 Created`). Setting `status: "done"` automatically sets `completed_at`.

### `GET /api/v1/todos/:id`
Fetches single todo (`400` bad UUID, `404` missing, `403` wrong owner).

### `PATCH /api/v1/todos/:id` & `PUT /api/v1/todos/:id`
Partially updates fields.
- Sending `"category_id": null` or `"due_date": null` explicitly clears the nullable column.
- Transitioning status to `"done"` populates `completed_at`.
- Transitioning status back to `"todo"` or `"in_progress"` clears `completed_at`.

### `DELETE /api/v1/todos/:id`
Deletes todo (`200 OK`, `{"deleted": true, "id": "..."}`).

### `POST /api/v1/todos/bulk`
Runs an atomic SQLite transaction on `1..100` user-owned todo IDs:
```json
{
  "action": "complete | reopen | archive | delete | set_priority",
  "ids": ["uuid-1", "uuid-2"],
  "priority": "urgent"
}
```
Response (`200 OK`): `{"action": "complete", "affected": 2, "requested": 2}`.
