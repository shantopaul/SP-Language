#!/usr/bin/env python3
"""Check that openapi.yaml matches the SP registry service's route table.

Verifies, in both directions, that every (method, path) in openapi.yaml
is registered in src/routes.sp and every registration in routes.sp
appears in openapi.yaml. Exits 0 on match, 1 with a diff otherwise.

Usage:
    python3 check_openapi.py [--routes src/routes.sp] [--spec openapi.yaml]
"""

import argparse
import re
import sys
from pathlib import Path

ROUTE_RE = re.compile(r'^\s*app\.(get|post)\("([^"]+)",')
SPEC_PATH_RE = re.compile(r"^  (/[^ :]+):\s*$")
SPEC_METHOD_RE = re.compile(r"^    (get|post|put|patch|delete):\s*$")


def routes_from_sp(path):
    found = set()
    for line in Path(path).read_text().splitlines():
        m = ROUTE_RE.match(line)
        if m:
            api_path = re.sub(r":([A-Za-z_][A-Za-z0-9_]*)", r"{\1}", m.group(2))
            found.add((m.group(1).upper(), api_path))
    return found


def routes_from_spec(path):
    found = set()
    current = None
    in_paths = False
    for line in Path(path).read_text().splitlines():
        if line == "paths:":
            in_paths = True
            continue
        if in_paths and re.match(r"^[a-z]", line):
            in_paths = False
        if not in_paths:
            continue
        m = SPEC_PATH_RE.match(line)
        if m:
            current = m.group(1)
            continue
        m = SPEC_METHOD_RE.match(line)
        if m and current:
            found.add((m.group(1).upper(), current))
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--routes", default=str(Path(__file__).parent / "src" / "routes.sp"))
    ap.add_argument("--spec", default=str(Path(__file__).parent / "openapi.yaml"))
    args = ap.parse_args()
    service = routes_from_sp(args.routes)
    spec = routes_from_spec(args.spec)
    ok = True
    only_service = sorted(service - spec)
    only_spec = sorted(spec - service)
    if only_service:
        ok = False
        print("in service but NOT in spec:")
        for m, p in only_service:
            print("  %s %s" % (m, p))
    if only_spec:
        ok = False
        print("in spec but NOT in service:")
        for m, p in only_spec:
            print("  %s %s" % (m, p))
    if ok:
        print("OK: openapi.yaml matches routes.sp (%d routes)" % len(service))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
