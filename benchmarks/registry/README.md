# Real HTTP load-test harness for SP services

`load_test.py` (Python 3, standard library only) fires real HTTP requests
at a target URL and reports **measured** requests/sec, latency percentiles
(p50/p95/p99), status-code counts, and errors. Every number it prints
comes from that run's measurements — nothing is hard-coded.

## Measure the SP registry service

```sh
# Start the service first (see examples/registry-backend/README.md)
python3 benchmarks/registry/load_test.py \
  --url http://127.0.0.1:8090/api/health \
  --concurrency 10 --duration 10 \
  --output benchmarks/registry/results/health-<date>.json
```

## Measure a publish (authenticated POST)

```sh
python3 benchmarks/registry/load_test.py \
  --url http://127.0.0.1:8090/api/v1/publish \
  --method POST --body-file /tmp/publish-payload.json \
  --header "Authorization: Bearer $SP_REGISTRY_TOKEN" \
  --header "Content-Type: application/json" \
  --requests 100 --concurrency 4
```

Note: each publish in that run must use a distinct package version, or
later requests will 409 (releases are immutable) — the harness counts
4xx responses in its status-code table instead of hiding them.

## What a run proves

- The target actually answered that many real requests in the measured
  wall time (throughput is `requests / elapsed`, computed per run).
- Latency percentiles are computed from the per-request timings of the
  run, after a short warmup that is excluded from the numbers.
- A failed target produces 0 successes and exit code 1 — never numbers.

## Recorded runs

See `docs/v2/PHASE-31-PRO-4.0.0.md` for the runs recorded so far. Do not
quote numbers from anywhere else: if a run is not in that doc (or in a
`results/*.json` file next to this README), it did not happen.
