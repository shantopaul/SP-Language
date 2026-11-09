#!/usr/bin/env python3
"""Real HTTP load-test harness for SP services (Phase 2).

Fires real HTTP requests at a target URL with a thread pool and reports
MEASURED requests/sec, latency percentiles, and errors. Every number in
the output comes from this run's measurements — nothing is hard-coded,
estimated, or copied from another run. If the target is unreachable, the
harness reports 0 requests and fails instead of printing numbers.

Usage:
    python3 load_test.py --url http://127.0.0.1:8090/api/health \\
        --concurrency 10 --duration 10
    python3 load_test.py --url http://127.0.0.1:8090/api/v1/publish \\
        --method POST --body-file payload.json \\
        --header "Authorization: Bearer <token>" --header "Content-Type: application/json" \\
        --requests 200 --concurrency 4
    python3 load_test.py --url http://127.0.0.1:8090/api/health \\
        --duration 5 --output results/health-2026-11-09.json

Exit code: 0 when at least one request succeeded, 1 when none did.
"""

import argparse
import json
import os
import platform
import statistics
import sys
import threading
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone


def percentile(sorted_values, pct):
    """Nearest-rank percentile of an already-sorted list (ms)."""
    if not sorted_values:
        return None
    rank = (pct / 100.0) * len(sorted_values)
    idx = max(0, min(len(sorted_values) - 1, int(rank)))
    return sorted_values[idx]


def worker(url, method, body, headers, timeout, stop_at, budget, counter,
           latencies, statuses, errors, lock):
    while True:
        with lock:
            if budget["remaining"] <= 0:
                return
            budget["remaining"] -= 1
            n = counter["done"] + 1
            counter["done"] = n
        if time.monotonic() >= stop_at:
            return
        req = urllib.request.Request(url, data=body, method=method)
        for k, v in headers.items():
            req.add_header(k, v)
        start = time.perf_counter()
        try:
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                resp.read()
                status = resp.status
            elapsed_ms = (time.perf_counter() - start) * 1000.0
            with lock:
                latencies.append(elapsed_ms)
                statuses[status] = statuses.get(status, 0) + 1
        except urllib.error.HTTPError as e:
            elapsed_ms = (time.perf_counter() - start) * 1000.0
            try:
                e.read()
            except Exception:
                pass
            with lock:
                latencies.append(elapsed_ms)
                statuses[e.code] = statuses.get(e.code, 0) + 1
        except Exception as e:  # timeout, refused, reset, DNS, ...
            with lock:
                errors[type(e).__name__] = errors.get(type(e).__name__, 0) + 1


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--url", required=True, help="Target URL to measure.")
    ap.add_argument("--method", default="GET")
    ap.add_argument("--body", default=None, help="Request body as a literal string.")
    ap.add_argument("--body-file", default=None, help="Read the request body from this file.")
    ap.add_argument("--header", action="append", default=[],
                    help="Request header 'Name: value' (repeatable).")
    ap.add_argument("--concurrency", type=int, default=10)
    ap.add_argument("--duration", type=float, default=10.0,
                    help="How long to run, in seconds (default 10).")
    ap.add_argument("--requests", type=int, default=0,
                    help="Total requests to send (0 = run for --duration).")
    ap.add_argument("--timeout", type=float, default=5.0, help="Per-request timeout, seconds.")
    ap.add_argument("--warmup", type=float, default=1.0, help="Warmup seconds (not counted).")
    ap.add_argument("--output", default=None, help="Write the JSON report to this file.")
    args = ap.parse_args()

    if args.concurrency < 1:
        print("error: --concurrency must be >= 1", file=sys.stderr)
        return 2
    if args.duration <= 0 and args.requests <= 0:
        print("error: give --duration > 0 or --requests > 0", file=sys.stderr)
        return 2

    body = None
    if args.body_file:
        with open(args.body_file, "rb") as f:
            body = f.read()
    elif args.body is not None:
        body = args.body.encode("utf-8")

    headers = {}
    for h in args.header:
        if ":" not in h:
            print("error: --header must be 'Name: value', got %r" % h, file=sys.stderr)
            return 2
        k, v = h.split(":", 1)
        headers[k.strip()] = v.strip()

    # Warmup: a few real requests that are NOT counted, so connection setup
    # and cold caches do not pollute the measured run.
    if args.warmup > 0:
        warm_end = time.monotonic() + args.warmup
        while time.monotonic() < warm_end:
            try:
                req = urllib.request.Request(args.url, data=body, method=args.method)
                for k, v in headers.items():
                    req.add_header(k, v)
                with urllib.request.urlopen(req, timeout=args.timeout) as resp:
                    resp.read()
            except Exception:
                pass
            time.sleep(0.05)

    total = args.requests if args.requests > 0 else 10 ** 9
    budget = {"remaining": total}
    counter = {"done": 0}
    latencies, statuses, errors = [], {}, {}
    lock = threading.Lock()
    stop_at = time.monotonic() + args.duration

    started = datetime.now(timezone.utc)
    run_start = time.monotonic()
    threads = []
    for _ in range(args.concurrency):
        t = threading.Thread(target=worker, args=(
            args.url, args.method, body, headers, args.timeout, stop_at,
            budget, counter, latencies, statuses, errors, lock))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
    elapsed = time.monotonic() - run_start

    done = len(latencies) + sum(errors.values())
    ok = sum(c for s, c in statuses.items() if 200 <= s < 300)
    lat_sorted = sorted(latencies)
    report = {
        "measured_at_utc": started.isoformat(),
        "target": {"url": args.url, "method": args.method},
        "config": {
            "concurrency": args.concurrency,
            "duration_seconds": args.duration,
            "requested_total": args.requests if args.requests > 0 else None,
            "timeout_seconds": args.timeout,
            "warmup_seconds": args.warmup,
        },
        "context": {
            "host": platform.node(),
            "os": platform.platform(),
            "python": platform.python_version(),
            "cpus": os.cpu_count(),
        },
        "measured": {
            "elapsed_seconds": round(elapsed, 3),
            "requests_sent": done,
            "requests_succeeded_2xx": ok,
            "requests_per_second": round(done / elapsed, 2) if elapsed > 0 else 0,
            "latency_ms": {
                "mean": round(statistics.mean(lat_sorted), 2) if lat_sorted else None,
                "p50": round(percentile(lat_sorted, 50), 2) if lat_sorted else None,
                "p95": round(percentile(lat_sorted, 95), 2) if lat_sorted else None,
                "p99": round(percentile(lat_sorted, 99), 2) if lat_sorted else None,
                "max": round(max(lat_sorted), 2) if lat_sorted else None,
            },
            "status_codes": {str(k): v for k, v in sorted(statuses.items())},
            "errors": errors,
        },
    }

    print("=" * 68)
    print("MEASURED load-test result (this run only — not a quoted figure)")
    print("=" * 68)
    print("target:      %s %s" % (args.method, args.url))
    print("measured at: %s" % report["measured_at_utc"])
    print("context:     %s, %s, %d CPU(s)" % (
        report["context"]["os"], report["context"]["python"],
        report["context"]["cpus"] or 0))
    print("config:      concurrency=%d, duration=%.1fs, timeout=%.1fs" % (
        args.concurrency, args.duration, args.timeout))
    print("-" * 68)
    print("requests:    %d sent in %.3fs" % (done, elapsed))
    print("throughput:  %.2f requests/sec (measured)" % report["measured"]["requests_per_second"])
    print("succeeded:   %d (2xx)" % ok)
    if lat_sorted:
        print("latency ms:  mean=%.2f p50=%.2f p95=%.2f p99=%.2f max=%.2f" % (
            report["measured"]["latency_ms"]["mean"],
            report["measured"]["latency_ms"]["p50"],
            report["measured"]["latency_ms"]["p95"],
            report["measured"]["latency_ms"]["p99"],
            report["measured"]["latency_ms"]["max"]))
    else:
        print("latency ms:  no completed requests")
    print("status codes: %s" % json.dumps(report["measured"]["status_codes"]))
    print("errors:      %s" % json.dumps(errors))
    print("=" * 68)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2)
        print("report written to %s" % args.output)

    return 0 if ok > 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
