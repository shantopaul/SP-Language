#!/usr/bin/env python3
"""Generate openapi.yaml for the SP registry service from its route table.

Reads examples/registry-backend/src/routes.sp and emits openapi.yaml next
to this script. The (method, path) pairs in the spec come ONLY from the
`app.get(...)` / `app.post(...)` registrations in routes.sp plus the
`// @openapi` annotations directly above them (summary, auth, params).
Nothing is invented: a route that is not registered in routes.sp cannot
appear in the spec.

Response codes are a transcription of handlers.sp: each route's handler
returns exactly the statuses listed in RESPONSES below. If you change a
handler's status codes, update RESPONSES and regenerate.

Usage:
    python3 generate_openapi.py [--routes src/routes.sp] [--out openapi.yaml]
"""

import argparse
import re
import sys
from pathlib import Path

# (method, path-template) -> {status: description}. Transcribed from
# src/handlers.sp — keep in sync with the handlers.
RESPONSES = {
    ("GET", "/api/health"): {
        "200": "Service is up; body reports the stored package count.",
    },
    ("POST", "/api/v1/publish"): {
        "201": "Package metadata stored in SQLite.",
        "400": "Invalid JSON, missing fields, bad name/version/checksum.",
        "401": "Missing or invalid Bearer token.",
        "409": "That name@version is already published (releases are immutable).",
        "500": "Registry storage error.",
    },
    ("POST", "/api/v1/packages/{name}/{version}/tarball"): {
        "200": "Tarball stored; its SHA-256 matches the published checksum.",
        "400": "Empty/invalid base64 body, or checksum mismatch.",
        "401": "Missing or invalid Bearer token.",
        "404": "No such package version; publish its metadata first.",
        "500": "Registry storage error.",
    },
    ("GET", "/api/v1/verify/{name}/{version}"): {
        "200": "Verification result computed from stored data.",
        "404": "No such package version.",
        "500": "Registry storage error.",
    },
    ("GET", "/api/v1/packages"): {
        "200": "List of all published packages.",
    },
    ("GET", "/api/v1/packages/{name}/{version}"): {
        "200": "Metadata for one package version.",
        "404": "No such package version.",
    },
    ("GET", "/api/v1/packages/{name}/{version}/download"): {
        "200": "Base64 tarball bytes (X-Content-Encoding: base64).",
        "404": "No such package version, or no tarball uploaded yet.",
    },
    ("GET", "/openapi.yaml"): {
        "200": "This OpenAPI specification.",
        "500": "Spec file not found on the server.",
    },
}

ROUTE_RE = re.compile(r'^\s*app\.(get|post)\("([^"]+)",')
ANN_RE = re.compile(r'^\s*// @openapi (\w+):\s*(.*)$')


def parse_routes(routes_path):
    """Return a list of dicts: method, path (OpenAPI style), summary, auth, params."""
    routes = []
    pending = {}
    for line in Path(routes_path).read_text().splitlines():
        m = ANN_RE.match(line)
        if m:
            key, val = m.group(1), m.group(2).strip()
            if key == "param":
                pending.setdefault("params", []).append(val)
            else:
                pending[key] = val
            continue
        m = ROUTE_RE.match(line)
        if m:
            method = m.group(1).upper()
            sp_path = m.group(2)
            api_path = re.sub(r":([A-Za-z_][A-Za-z0-9_]*)", r"{\1}", sp_path)
            params = []
            for p in pending.get("params", []):
                parts = p.split()
                if len(parts) == 3:
                    params.append({"name": parts[0], "in": parts[1], "type": parts[2]})
            routes.append({
                "method": method,
                "path": api_path,
                "summary": pending.get("summary", ""),
                "auth": pending.get("auth", "none"),
                "params": params,
            })
            pending = {}
    return routes


def q(s):
    """YAML double-quoted scalar (safe for colons and other punctuation)."""
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def emit(routes):
    out = []
    out.append("openapi: 3.0.3")
    out.append("info:")
    out.append("  title: SP Registry API")
    out.append("  version: 4.0.0")
    out.append("  description: >-")
    out.append("    Package registry backend implemented as an SP program on the SP")
    out.append("    HTTP server + SQLite (Phase 2 dogfooding). This spec is GENERATED")
    out.append("    from examples/registry-backend/src/routes.sp by generate_openapi.py;")
    out.append("    every path here is a route the service actually registers.")
    out.append("    The Next.js registry (splang-registry) is a separate labelled")
    out.append("    preview/mock and is NOT described by this spec.")
    out.append("servers:")
    out.append("  - url: http://127.0.0.1:8090")
    out.append("    description: Local SP registry service (default REGISTRY_PORT)")
    out.append("paths:")
    for r in routes:
        out.append(f"  {r['path']}:")
        out.append(f"    {r['method'].lower()}:")
        if r["summary"]:
            out.append(f"      summary: {q(r['summary'])}")
        if r["params"]:
            out.append("      parameters:")
            for p in r["params"]:
                out.append(f"        - name: {p['name']}")
                out.append(f"          in: {p['in']}")
                out.append("          required: true")
                out.append("          schema:")
                out.append(f"            type: {p['type']}")
        if r["auth"] == "bearer":
            out.append("      security:")
            out.append("        - BearerAuth: []")
        out.append("      responses:")
        for status, desc in RESPONSES[(r["method"], r["path"])].items():
            out.append(f"        '{status}':")
            out.append(f"          description: {q(desc)}")
    out.append("components:")
    out.append("  securitySchemes:")
    out.append("    BearerAuth:")
    out.append("      type: http")
    out.append("      scheme: bearer")
    out.append("      bearerFormat: opaque")
    out.append("      description: >-")
    out.append("        Publish token from SP_REGISTRY_TOKEN. Compared with a")
    out.append("        timing-safe equality check; missing or wrong tokens get 401.")
    out.append("")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--routes", default=str(Path(__file__).parent / "src" / "routes.sp"))
    ap.add_argument("--out", default=str(Path(__file__).parent / "openapi.yaml"))
    args = ap.parse_args()
    routes = parse_routes(args.routes)
    if not routes:
        print("error: no routes parsed from %s" % args.routes, file=sys.stderr)
        return 1
    missing = [r for r in routes if (r["method"], r["path"]) not in RESPONSES]
    if missing:
        print("error: no RESPONSES entry for: %s" % missing, file=sys.stderr)
        return 1
    Path(args.out).write_text(emit(routes))
    print("wrote %s (%d routes)" % (args.out, len(routes)))
    for r in routes:
        print("  %s %s" % (r["method"], r["path"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
