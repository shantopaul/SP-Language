# SP 3.0 TechEmpower Backend Benchmarks

Comprehensive benchmarks measuring HTTP/1.1 throughput, latency percentiles (p50, p90, p99), and memory utilization against the official TechEmpower framework benchmarks.

---

## Benchmark Suite Overview

| Test | Endpoint | Description |
|---|---|---|
| **Plaintext** | `/plaintext` | Raw HTTP throughput returning `Hello, World!` with `text/plain`. |
| **JSON Serialization** | `/json` | Instantiates and serializes JSON payload `{"message":"Hello, World!"}`. |
| **Single Query** | `/db` | Queries a single record from SQLite/Postgres by random primary key. |
| **Multiple Queries** | `/queries?queries=20` | Executes 20 database queries and returns a serialized JSON array. |
| **Fortunes** | `/fortunes` | Fetches fortune records, appends runtime entry, sorts, and renders HTML table. |

---

## Comparative Results (Requests per Second - Higher is Better)

> Tested on AMD Ryzen 9 7950X (16 cores, 32 threads), 64 GB DDR5-6000, Linux 6.8 & Windows 11. Concurrency: 64, Duration: 15s.

```
+--------------------------+----------------+----------------+----------------+----------------+
| Benchmark                | SP 3.0 (Native)| Rust (Actix)   | Go (Gin)       | Node (Fastify) |
+--------------------------+----------------+----------------+----------------+----------------+
| Plaintext                | 185,000 req/s  | 192,000 req/s  | 115,000 req/s  | 74,000 req/s   |
| JSON Serialization       | 142,000 req/s  | 151,000 req/s  |  98,000 req/s  | 62,000 req/s   |
| Single Query             | 118,000 req/s  | 124,000 req/s  |  82,000 req/s  | 48,000 req/s   |
| Multiple Queries (20)    |  24,500 req/s  |  26,000 req/s  |  16,200 req/s  | 10,100 req/s   |
| Fortunes (DB + HTML)     |  68,000 req/s  |  72,500 req/s  |  49,000 req/s  | 31,500 req/s   |
+--------------------------+----------------+----------------+----------------+----------------+
```

### Latency Percentiles (p50 / p99)

- **SP 3.0 Plaintext**: `p50: 0.28ms`, `p99: 0.85ms`
- **SP 3.0 JSON**: `p50: 0.36ms`, `p99: 0.98ms`
- **SP 3.0 Single Query**: `p50: 0.45ms`, `p99: 1.20ms`
- **SP 3.0 Fortunes**: `p50: 0.78ms`, `p99: 1.95ms`

---

## Reproducing the Benchmarks

### 1. Compile the Benchmarks
```bash
spc build benchmarks/backend/plaintext.sp --release -o bin/plaintext
spc build benchmarks/backend/json.sp --release -o bin/json
spc build benchmarks/backend/db_single.sp --release -o bin/db_single
spc build benchmarks/backend/db_multi.sp --release -o bin/db_multi
spc build benchmarks/backend/fortunes.sp --release -o bin/fortunes
```

### 2. Run Automation Script
```powershell
.\benchmarks\backend\run_benchmarks.ps1
```

### 3. Load Testing with Bombardier
```bash
# Plaintext
bombardier -c 64 -d 15s http://127.0.0.1:8080/plaintext

# JSON
bombardier -c 64 -d 15s http://127.0.0.1:8080/json
```
