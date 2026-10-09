#include <gtest/gtest.h>
#include "sp/runtime/Log.h"
#include "sp/runtime/Metrics.h"
#include "sp/runtime/Health.h"
#include "sp/runtime/Queue.h"
#include "sp/runtime/Runtime.h"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <string>
#include <filesystem>
#include <fstream>

using namespace sp::rt::obs;

// ============================================================================
// 1. Structured Logging (std.log)
// ============================================================================

TEST(ObservabilityTest, LoggerJsonFormatAndSeverityFiltering) {
    auto logger = std::make_shared<Logger>("test-logger", LogLevel::Info);
    auto memAppender = std::make_shared<MemoryAppender>(10);
    logger->addAppender(memAppender);

    // Debug message should be filtered out because level is Info
    logger->debug("This should not be logged");
    EXPECT_EQ(memAppender->size(), 0);

    // Info message should be logged
    logger->info("Service started");
    EXPECT_EQ(memAppender->size(), 1);

    std::string record = memAppender->get(0);
    EXPECT_NE(record.find("\"level\":\"INFO\""), std::string::npos);
    EXPECT_NE(record.find("\"logger\":\"test-logger\""), std::string::npos);
    EXPECT_NE(record.find("\"message\":\"Service started\""), std::string::npos);
    EXPECT_NE(record.find("\"timestamp\":\""), std::string::npos);

    // Warn with context
    logger->log(LogLevel::Warn, "Disk space low", {{"freeMb", "512"}, {"mount", "/var"}});
    EXPECT_EQ(memAppender->size(), 2);
    std::string warnRecord = memAppender->get(1);
    EXPECT_NE(warnRecord.find("\"level\":\"WARN\""), std::string::npos);
    EXPECT_NE(warnRecord.find("\"freeMb\":\"512\""), std::string::npos);
    EXPECT_NE(warnRecord.find("\"mount\":\"/var\""), std::string::npos);
}

TEST(ObservabilityTest, LoggerContextInheritanceWith) {
    auto rootLogger = std::make_shared<Logger>("api-server", LogLevel::Debug);
    auto memAppender = std::make_shared<MemoryAppender>(10);
    rootLogger->addAppender(memAppender);

    auto reqLogger = rootLogger->with("requestId", "req_9901")->with("userId", "usr_42");
    reqLogger->info("Authenticated user");

    ASSERT_EQ(memAppender->size(), 1);
    std::string rec = memAppender->get(0);
    EXPECT_NE(rec.find("\"requestId\":\"req_9901\""), std::string::npos);
    EXPECT_NE(rec.find("\"userId\":\"usr_42\""), std::string::npos);
    EXPECT_NE(rec.find("\"message\":\"Authenticated user\""), std::string::npos);
}

TEST(ObservabilityTest, LoggerMemoryAppenderCapacityEviction) {
    auto memAppender = std::make_shared<MemoryAppender>(3);
    LogRecord r1{"2026-01-01T00:00:00Z", LogLevel::Info, "logger", "log1", {}};
    LogRecord r2{"2026-01-01T00:00:01Z", LogLevel::Info, "logger", "log2", {}};
    LogRecord r3{"2026-01-01T00:00:02Z", LogLevel::Info, "logger", "log3", {}};
    LogRecord r4{"2026-01-01T00:00:03Z", LogLevel::Info, "logger", "log4", {}};

    memAppender->append(r1.toJson(), r1);
    memAppender->append(r2.toJson(), r2);
    memAppender->append(r3.toJson(), r3);
    EXPECT_EQ(memAppender->size(), 3);
    EXPECT_NE(memAppender->get(0).find("log1"), std::string::npos);

    // Appending 4th item evicts oldest (log1)
    memAppender->append(r4.toJson(), r4);
    EXPECT_EQ(memAppender->size(), 3);
    EXPECT_NE(memAppender->get(0).find("log2"), std::string::npos);
    EXPECT_NE(memAppender->get(2).find("log4"), std::string::npos);
}

TEST(ObservabilityTest, LoggerRotatingFileAppender) {
    std::string testFile = "temp_obs_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }

    {
        auto fileAppender = std::make_shared<RotatingFileAppender>(testFile, 500, 2);
        LogRecord rec{"2026-10-09T08:00:00Z", LogLevel::Error, "db-driver", "Database unreachable", {}};
        fileAppender->append(rec.toJson(), rec);
    }

    EXPECT_TRUE(std::filesystem::exists(testFile));
    std::string content;
    {
        std::ifstream in(testFile);
        content.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    EXPECT_NE(content.find("\"Database unreachable\""), std::string::npos);
    EXPECT_NE(content.find("\"db-driver\""), std::string::npos);

    std::filesystem::remove(testFile);
}

// ============================================================================
// 2. Prometheus Exposition Metrics (std.metrics)
// ============================================================================

TEST(ObservabilityTest, MetricsCounterGaugeHistogram) {
    MetricsRegistry registry;

    auto httpRequests = registry.counter("http_requests_total", "Total HTTP requests");
    httpRequests->inc({{"method", "GET"}, {"handler", "/api/v1/users"}}, 5.0);
    httpRequests->inc({{"method", "POST"}, {"handler", "/api/v1/users"}}, 2.0);

    auto activeConns = registry.gauge("db_connections_active", "Active DB connections");
    activeConns->set(10.0);
    activeConns->add(2.0);
    activeConns->add(-3.0);

    auto latencies = registry.histogram("request_latency_seconds", "Request latency", {0.01, 0.05, 0.1, 0.5, 1.0});
    latencies->observe(0.02, {{"status", "200"}});
    latencies->observe(0.08, {{"status", "200"}});
    latencies->observe(0.60, {{"status", "200"}});

    std::string prometheusOutput = registry.renderPrometheusText();

    // Verify Counter exposition
    EXPECT_NE(prometheusOutput.find("# HELP http_requests_total Total HTTP requests"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("# TYPE http_requests_total counter"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("http_requests_total{handler=\"/api/v1/users\",method=\"GET\"} 5"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("http_requests_total{handler=\"/api/v1/users\",method=\"POST\"} 2"), std::string::npos);

    // Verify Gauge exposition
    EXPECT_NE(prometheusOutput.find("# HELP db_connections_active Active DB connections"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("# TYPE db_connections_active gauge"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("db_connections_active 9"), std::string::npos);

    // Verify Histogram exposition
    EXPECT_NE(prometheusOutput.find("# TYPE request_latency_seconds histogram"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("request_latency_seconds_bucket{le=\"0.01\",status=\"200\"} 0"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("request_latency_seconds_bucket{le=\"0.05\",status=\"200\"} 1"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("request_latency_seconds_bucket{le=\"0.1\",status=\"200\"} 2"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("request_latency_seconds_bucket{le=\"1\",status=\"200\"} 3"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("request_latency_seconds_bucket{le=\"+Inf\",status=\"200\"} 3"), std::string::npos);
    EXPECT_NE(prometheusOutput.find("request_latency_seconds_count{status=\"200\"} 3"), std::string::npos);
}

// ============================================================================
// 3. Health & Readiness Telemetry (std.health)
// ============================================================================

TEST(ObservabilityTest, HealthRegistryLivenessAndReadiness) {
    HealthRegistry health;

    // Liveness probe should be UP by default
    std::string liveJson = health.checkLiveness();
    EXPECT_NE(liveJson.find("\"status\":\"UP\""), std::string::npos);
    EXPECT_NE(liveJson.find("\"timestamp\":\""), std::string::npos);

    // Register readiness checks
    health.setStatus("postgres", true, "10/10 pool active");
    health.setStatus("redis", true, "ping < 1ms");

    auto [isReady, readyJson] = health.checkReadiness();
    EXPECT_TRUE(isReady);
    EXPECT_NE(readyJson.find("\"status\":\"UP\""), std::string::npos);
    EXPECT_NE(readyJson.find("\"postgres\":{\"status\":\"UP\""), std::string::npos);
    EXPECT_NE(readyJson.find("\"redis\":{\"status\":\"UP\""), std::string::npos);

    // Mark redis down -> overall readiness becomes DOWN
    health.setStatus("redis", false, "connection refused");
    auto [isDownReady, downReadyJson] = health.checkReadiness();
    EXPECT_FALSE(isDownReady);
    EXPECT_NE(downReadyJson.find("\"status\":\"DOWN\""), std::string::npos);
    EXPECT_NE(downReadyJson.find("\"redis\":{\"status\":\"DOWN\""), std::string::npos);
}

// ============================================================================
// 4. Background Job Queue & Retries (std.queue)
// ============================================================================

TEST(ObservabilityTest, JobQueueExecutionAndRetryDLQ) {
    JobQueue queue(2);

    std::atomic<int> successCount{0};
    std::atomic<int> attemptsCount{0};

    // Handler 1: always succeeds
    queue.registerHandler("process_order", [&](const Job& job, std::string& err) -> bool {
        (void)job; (void)err;
        successCount++;
        return true;
    });

    // Handler 2: always fails to test retries and dead-letter queue
    queue.registerHandler("failing_task", [&](const Job& job, std::string& err) -> bool {
        (void)job;
        attemptsCount++;
        err = "simulated database crash";
        return false;
    });

    // Enqueue 2 successful jobs
    queue.enqueue("process_order", "{\"id\":\"ord_1\"}", 1, 10, 0);
    queue.enqueue("process_order", "{\"id\":\"ord_2\"}", 1, 10, 0);

    // Enqueue 1 failing job with maxRetries = 2, backoffMs = 10
    queue.enqueue("failing_task", "{\"payload\":\"test\"}", 2, 10, 0);

    queue.waitIdle();

    EXPECT_EQ(successCount.load(), 2);
    EXPECT_EQ(attemptsCount.load(), 2); // Initial attempt + 1 retry = 2 attempts
    EXPECT_EQ(queue.completedCount(), 2);
    EXPECT_EQ(queue.dlqCount(), 1);

    // Inspect DLQ job
    std::string dlqJobJson = queue.getDlqJobJson(0);
    EXPECT_NE(dlqJobJson.find("\"failing_task\""), std::string::npos);
    EXPECT_NE(dlqJobJson.find("\"dead\""), std::string::npos);
    EXPECT_NE(dlqJobJson.find("simulated database crash"), std::string::npos);

    // Retry from DLQ
    bool retried = queue.retryDlqJob("job_3");
    EXPECT_TRUE(retried);
    queue.waitIdle();

    // After retry fails again, it goes back to DLQ
    EXPECT_EQ(queue.dlqCount(), 1);

    queue.clearDlq();
    EXPECT_EQ(queue.dlqCount(), 0);

    queue.stop();
}

// ============================================================================
// 5. Cron Scheduler (std.queue)
// ============================================================================

TEST(ObservabilityTest, CronSchedulerIntervalExecution) {
    CronScheduler cron;

    std::atomic<int> tickCount{0};
    cron.scheduleInterval("fast_ticker", 20, [&]() {
        tickCount++;
    });

    cron.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    cron.stop();

    // In 120ms with 20ms interval, should tick at least 3 times
    EXPECT_GE(tickCount.load(), 3);
    EXPECT_EQ(cron.taskCount(), 1);
}

// ============================================================================
// 6. C-ABI Bridge Functionality
// ============================================================================

TEST(ObservabilityTest, CAbiObservabilityBridges) {
    // 1. Log bridge
    void* logger = sp_rt_logger_create("cabi-log", 1); // 1 = Debug
    ASSERT_NE(logger, nullptr);
    void* memAppender = sp_rt_logger_add_memory(logger, 10);
    ASSERT_NE(memAppender, nullptr);

    sp_rt_logger_log(logger, 2, "C-ABI info log", "{\"env\":\"production\"}");
    EXPECT_EQ(sp_rt_memory_appender_size(memAppender), 1);
    const char* logJson = sp_rt_memory_appender_get(memAppender, 0);
    EXPECT_NE(std::string(logJson).find("C-ABI info log"), std::string::npos);
    EXPECT_NE(std::string(logJson).find("\"env\":\"production\""), std::string::npos);

    sp_rt_memory_appender_destroy(memAppender);
    sp_rt_logger_destroy(logger);

    // 2. Metrics bridge
    void* reg = sp_rt_metrics_registry_create();
    ASSERT_NE(reg, nullptr);
    sp_rt_metrics_counter_inc(reg, "api_hits", "API hits", "{\"route\":\"/home\"}", 10.0);
    sp_rt_metrics_gauge_set(reg, "active_users", "Active users", "", 42.0);
    const char* promText = sp_rt_metrics_render_prometheus(reg);
    EXPECT_NE(std::string(promText).find("api_hits{route=\"/home\"} 10"), std::string::npos);
    EXPECT_NE(std::string(promText).find("active_users 42"), std::string::npos);
    sp_rt_metrics_registry_destroy(reg);

    // 3. Health bridge
    void* healthReg = sp_rt_health_registry_create();
    ASSERT_NE(healthReg, nullptr);
    sp_rt_health_set_status(healthReg, "cache", true, "ok");
    bool isUp = false;
    const char* readiness = sp_rt_health_check_readiness(healthReg, &isUp);
    EXPECT_TRUE(isUp);
    EXPECT_NE(std::string(readiness).find("\"cache\":{\"status\":\"UP\""), std::string::npos);
    sp_rt_health_registry_destroy(healthReg);

    // 4. Job queue bridge
    void* q = sp_rt_job_queue_create(1);
    ASSERT_NE(q, nullptr);
    sp_rt_job_queue_stop(q);
    const char* jobId = sp_rt_job_queue_enqueue(q, "test_job", "{\"data\":\"1\"}", 1, 10, 0);
    EXPECT_NE(jobId, nullptr);
    EXPECT_EQ(sp_rt_job_queue_pending_count(q), 1);
    sp_rt_job_queue_start(q);
    sp_rt_job_queue_wait_idle(q);
    EXPECT_EQ(sp_rt_job_queue_dlq_count(q), 1); // no handler registered -> went to DLQ
    sp_rt_job_queue_destroy(q);
}
