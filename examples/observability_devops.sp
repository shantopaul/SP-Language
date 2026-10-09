module examples.observability_devops;

import std.io;
import std.log;
import std.metrics;
import std.health;
import std.queue;

fn main() {
    println("====================================================================");
    println(" SP Production Observability, Background Workers & Telemetry (2.9)  ");
    println("====================================================================");

    // -------------------------------------------------------------------------
    // 1. Structured Logging (std.log)
    // -------------------------------------------------------------------------
    println("\n[1] Testing Structured Logging (JSON formatting & contexts):");
    let logger = Logger.create("orders-service", Logger.levelDebug());
    let memAppender = logger.addMemory(50);

    logger.info("Service initialized successfully");
    logger.debug("Debug profiling trace active");

    let userLogger = logger.withField("userId", "usr_9981");
    let reqLogger = userLogger.withField("traceId", "trc_4481_abc");
    reqLogger.info("Processed checkout payment successfully");

    print("Logged entries captured in memory appender: ");
    println(memAppender.size());
    println("First memory record sample:");
    println(memAppender.get(0));

    // -------------------------------------------------------------------------
    // 2. Prometheus Exposition Metrics (std.metrics)
    // -------------------------------------------------------------------------
    println("\n[2] Testing Prometheus Metrics (Counter, Gauge, Histogram):");
    let httpReqs = Metrics.counterWithHelp("http_requests_total", "Total incoming HTTP requests");
    httpReqs.incWith(1.0, "{\"method\":\"GET\",\"status\":\"200\"}");
    httpReqs.incWith(1.0, "{\"method\":\"POST\",\"status\":\"201\"}");
    httpReqs.incWith(2.0, "{\"method\":\"GET\",\"status\":\"200\"}");

    let memoryGauge = Metrics.gaugeWithHelp("process_resident_memory_bytes", "Resident memory bytes");
    memoryGauge.set(16777216.0);
    memoryGauge.inc(2048.0);

    let latencyHist = Metrics.histogramWithHelp("http_request_duration_seconds", "HTTP latency in seconds");
    latencyHist.observeWith(0.003, "{\"route\":\"/api/v1/orders\"}");
    latencyHist.observeWith(0.012, "{\"route\":\"/api/v1/orders\"}");
    latencyHist.observeWith(0.150, "{\"route\":\"/api/v1/orders\"}");

    println("Prometheus Exposition Text Format (/metrics output):");
    println("----------------------------------------------------");
    println(Metrics.render());
    println("----------------------------------------------------");

    // -------------------------------------------------------------------------
    // 3. Health & Readiness Telemetry (std.health)
    // -------------------------------------------------------------------------
    println("\n[3] Testing Health & Readiness Probes (/healthz and /readyz):");
    Health.setStatus("postgres_primary", true, "connection pool healthy (4/10 active)");
    Health.setStatus("redis_cache", true, "cluster ping latency < 1ms");

    println("Liveness Probe (/healthz):");
    println(Health.liveness());

    println("Readiness Probe (/readyz):");
    println(Health.readiness());

    // -------------------------------------------------------------------------
    // 4. Background Job Queue & Workers (std.queue)
    // -------------------------------------------------------------------------
    println("\n[4] Testing Background Worker Job Queue with Retry & DLQ:");
    let queue = JobQueue.create(2);
    queue.start();

    let job1 = queue.enqueue("send_email", "{\"to\":\"alice@example.com\"}", 1, 10, 0);
    let job2 = queue.enqueue("generate_invoice", "{\"orderId\":\"ord_123\"}", 1, 10, 0);

    println("Enqueued jobs: " + job1 + ", " + job2);
    print("Active pending count: ");
    println(queue.pendingCount());

    // Wait for jobs to settle
    queue.waitIdle();
    print("Completed jobs: ");
    println(queue.completedCount());
    print("Dead letter queue (DLQ) count: ");
    println(queue.dlqCount());

    queue.stop();

    println("\nObservability, Queue & Telemetry showcase executed successfully!");
}
