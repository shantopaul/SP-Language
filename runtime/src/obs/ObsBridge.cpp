#include "sp/runtime/Log.h"
#include "sp/runtime/Metrics.h"
#include "sp/runtime/Health.h"
#include "sp/runtime/Queue.h"
#include "sp/runtime/Runtime.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <sstream>

using namespace sp::rt::obs;

namespace {

const char* alloc_str(const std::string& s) {
    char* res = static_cast<char*>(std::malloc(s.size() + 1));
    if (res) {
        std::memcpy(res, s.c_str(), s.size() + 1);
    }
    return res;
}

std::vector<double> parseBucketsCsv(const char* csv) {
    std::vector<double> buckets;
    if (!csv || !*csv) return buckets;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
            try {
                buckets.push_back(std::stod(item));
            } catch (...) {}
        }
    }
    return buckets;
}

} // anonymous namespace

extern "C" {

// ============================================================================
// Logger C-ABI
// ============================================================================

void* sp_rt_logger_create(const char* name, int32_t level) {
    auto logger = std::make_shared<Logger>(name ? name : "app", static_cast<LogLevel>(level));
    return new std::shared_ptr<Logger>(std::move(logger));
}

void sp_rt_logger_destroy(void* logger) {
    if (logger) {
        delete static_cast<std::shared_ptr<Logger>*>(logger);
    }
}

void* sp_rt_logger_default() {
    return new std::shared_ptr<Logger>(Logger::getDefault());
}

void sp_rt_logger_set_level(void* logger, int32_t level) {
    if (logger) {
        auto* l = static_cast<std::shared_ptr<Logger>*>(logger);
        if (*l) (*l)->setLevel(static_cast<LogLevel>(level));
    }
}

int32_t sp_rt_logger_get_level(void* logger) {
    if (logger) {
        auto* l = static_cast<std::shared_ptr<Logger>*>(logger);
        if (*l) return static_cast<int32_t>((*l)->getLevel());
    }
    return static_cast<int32_t>(LogLevel::Info);
}

void sp_rt_logger_add_console(void* logger) {
    if (logger) {
        auto* l = static_cast<std::shared_ptr<Logger>*>(logger);
        if (*l) (*l)->addAppender(std::make_shared<ConsoleAppender>());
    }
}

void sp_rt_logger_add_file(void* logger, const char* filepath, int32_t max_bytes, int32_t max_backups) {
    if (logger && filepath) {
        auto* l = static_cast<std::shared_ptr<Logger>*>(logger);
        if (*l) (*l)->addAppender(std::make_shared<RotatingFileAppender>(filepath, max_bytes, max_backups));
    }
}

void* sp_rt_logger_add_memory(void* logger, int32_t capacity) {
    if (!logger) return nullptr;
    auto* l = static_cast<std::shared_ptr<Logger>*>(logger);
    if (!*l) return nullptr;
    auto memAppender = std::make_shared<MemoryAppender>(capacity > 0 ? capacity : 1000);
    (*l)->addAppender(memAppender);
    return new std::shared_ptr<MemoryAppender>(std::move(memAppender));
}

void sp_rt_logger_log(void* logger, int32_t level, const char* message, const char* fields_json) {
    if (!logger || !message) return;
    auto* l = static_cast<std::shared_ptr<Logger>*>(logger);
    if (!*l) return;

    std::vector<std::pair<std::string, std::string>> extra;
    if (fields_json && *fields_json) {
        Labels lbls = parseLabelsJson(fields_json);
        extra.assign(lbls.begin(), lbls.end());
    }
    (*l)->log(static_cast<LogLevel>(level), message, extra);
}

void* sp_rt_logger_with(void* logger, const char* key, const char* val) {
    if (!logger || !key) return nullptr;
    auto* l = static_cast<std::shared_ptr<Logger>*>(logger);
    if (!*l) return nullptr;
    auto child = (*l)->with(key, val ? val : "");
    return new std::shared_ptr<Logger>(std::move(child));
}

const char* sp_rt_memory_appender_get(void* appender, int32_t index) {
    if (!appender || index < 0) return alloc_str("");
    auto* a = static_cast<std::shared_ptr<MemoryAppender>*>(appender);
    if (!*a) return alloc_str("");
    return alloc_str((*a)->get(static_cast<size_t>(index)));
}

int32_t sp_rt_memory_appender_size(void* appender) {
    if (!appender) return 0;
    auto* a = static_cast<std::shared_ptr<MemoryAppender>*>(appender);
    if (!*a) return 0;
    return static_cast<int32_t>((*a)->size());
}

void sp_rt_memory_appender_clear(void* appender) {
    if (!appender) return;
    auto* a = static_cast<std::shared_ptr<MemoryAppender>*>(appender);
    if (*a) (*a)->clear();
}

void sp_rt_memory_appender_destroy(void* appender) {
    if (appender) {
        delete static_cast<std::shared_ptr<MemoryAppender>*>(appender);
    }
}

// ============================================================================
// Metrics C-ABI
// ============================================================================

void* sp_rt_metrics_registry_create() {
    return new MetricsRegistry();
}

void sp_rt_metrics_registry_destroy(void* reg) {
    if (reg) {
        delete static_cast<MetricsRegistry*>(reg);
    }
}

void* sp_rt_metrics_registry_default() {
    return &MetricsRegistry::getGlobal();
}

void sp_rt_metrics_counter_inc(void* reg, const char* name, const char* help, const char* labels_json, double val) {
    if (!reg || !name) return;
    auto* r = static_cast<MetricsRegistry*>(reg);
    auto c = r->counter(name, help ? help : "");
    Labels lbls = (labels_json && *labels_json) ? parseLabelsJson(labels_json) : Labels{};
    c->inc(lbls, val);
}

void sp_rt_metrics_gauge_set(void* reg, const char* name, const char* help, const char* labels_json, double val) {
    if (!reg || !name) return;
    auto* r = static_cast<MetricsRegistry*>(reg);
    auto g = r->gauge(name, help ? help : "");
    Labels lbls = (labels_json && *labels_json) ? parseLabelsJson(labels_json) : Labels{};
    g->set(val, lbls);
}

void sp_rt_metrics_gauge_add(void* reg, const char* name, const char* help, const char* labels_json, double val) {
    if (!reg || !name) return;
    auto* r = static_cast<MetricsRegistry*>(reg);
    auto g = r->gauge(name, help ? help : "");
    Labels lbls = (labels_json && *labels_json) ? parseLabelsJson(labels_json) : Labels{};
    g->add(val, lbls);
}

void sp_rt_metrics_histogram_observe(void* reg, const char* name, const char* help, const char* buckets_csv, const char* labels_json, double val) {
    if (!reg || !name) return;
    auto* r = static_cast<MetricsRegistry*>(reg);
    std::vector<double> buckets = parseBucketsCsv(buckets_csv);
    auto h = r->histogram(name, help ? help : "", buckets);
    Labels lbls = (labels_json && *labels_json) ? parseLabelsJson(labels_json) : Labels{};
    h->observe(val, lbls);
}

const char* sp_rt_metrics_render_prometheus(void* reg) {
    if (!reg) return alloc_str("");
    auto* r = static_cast<MetricsRegistry*>(reg);
    return alloc_str(r->renderPrometheusText());
}

// ============================================================================
// Health C-ABI
// ============================================================================

void* sp_rt_health_registry_create() {
    return new HealthRegistry();
}

void sp_rt_health_registry_destroy(void* reg) {
    if (reg) {
        delete static_cast<HealthRegistry*>(reg);
    }
}

void* sp_rt_health_registry_default() {
    return &HealthRegistry::getGlobal();
}

void sp_rt_health_register_check(void* reg, const char* name, bool (*check_fn)()) {
    if (!reg || !name || !check_fn) return;
    auto* r = static_cast<HealthRegistry*>(reg);
    r->registerCheck(name, [check_fn]() -> HealthCheckResult {
        bool ok = check_fn();
        return HealthCheckResult{ok, ok ? "UP" : "check failed"};
    });
}

void sp_rt_health_set_status(void* reg, const char* name, bool is_up, const char* message) {
    if (!reg || !name) return;
    auto* r = static_cast<HealthRegistry*>(reg);
    r->setStatus(name, is_up, message ? message : "");
}

const char* sp_rt_health_check_liveness(void* reg) {
    if (!reg) return alloc_str("{\"status\":\"DOWN\"}");
    auto* r = static_cast<HealthRegistry*>(reg);
    return alloc_str(r->checkLiveness());
}

const char* sp_rt_health_check_readiness(void* reg, bool* out_is_up) {
    if (!reg) {
        if (out_is_up) *out_is_up = false;
        return alloc_str("{\"status\":\"DOWN\"}");
    }
    auto* r = static_cast<HealthRegistry*>(reg);
    auto [isUp, jsonStr] = r->checkReadiness();
    if (out_is_up) *out_is_up = isUp;
    return alloc_str(jsonStr);
}

// ============================================================================
// Queue C-ABI
// ============================================================================

void* sp_rt_job_queue_create(int32_t worker_count) {
    return new JobQueue(worker_count > 0 ? worker_count : 2);
}

void sp_rt_job_queue_destroy(void* queue) {
    if (queue) {
        delete static_cast<JobQueue*>(queue);
    }
}

void* sp_rt_job_queue_default() {
    return &JobQueue::getGlobal();
}

void sp_rt_job_queue_start(void* queue) {
    if (queue) static_cast<JobQueue*>(queue)->start();
}

void sp_rt_job_queue_stop(void* queue) {
    if (queue) static_cast<JobQueue*>(queue)->stop();
}

void sp_rt_job_queue_wait_idle(void* queue) {
    if (queue) static_cast<JobQueue*>(queue)->waitIdle();
}

const char* sp_rt_job_queue_enqueue(void* queue, const char* name, const char* payload,
                                int32_t max_retries, int32_t backoff_ms, int32_t delay_ms) {
    if (!queue || !name) return alloc_str("");
    auto* q = static_cast<JobQueue*>(queue);
    std::string id = q->enqueue(name, payload ? payload : "", max_retries, backoff_ms, delay_ms);
    return alloc_str(id);
}

void sp_rt_job_queue_register_handler(void* queue, const char* name, bool (*handler_fn)(const char*, const char*, const char*)) {
    if (!queue || !name || !handler_fn) return;
    auto* q = static_cast<JobQueue*>(queue);
    q->registerHandler(name, [handler_fn](const Job& job, std::string& outErr) -> bool {
        bool ok = handler_fn(job.id.c_str(), job.name.c_str(), job.payload.c_str());
        if (!ok) {
            outErr = "handler returned false";
        }
        return ok;
    });
}

int32_t sp_rt_job_queue_pending_count(void* queue) {
    if (!queue) return 0;
    return static_cast<int32_t>(static_cast<JobQueue*>(queue)->pendingCount());
}

int32_t sp_rt_job_queue_active_count(void* queue) {
    if (!queue) return 0;
    return static_cast<int32_t>(static_cast<JobQueue*>(queue)->activeCount());
}

int32_t sp_rt_job_queue_completed_count(void* queue) {
    if (!queue) return 0;
    return static_cast<int32_t>(static_cast<JobQueue*>(queue)->completedCount());
}

int32_t sp_rt_job_queue_dlq_count(void* queue) {
    if (!queue) return 0;
    return static_cast<int32_t>(static_cast<JobQueue*>(queue)->dlqCount());
}

const char* sp_rt_job_queue_dlq_get(void* queue, int32_t index) {
    if (!queue || index < 0) return alloc_str("{}");
    return alloc_str(static_cast<JobQueue*>(queue)->getDlqJobJson(static_cast<size_t>(index)));
}

bool sp_rt_job_queue_retry_dlq(void* queue, const char* job_id) {
    if (!queue || !job_id) return false;
    return static_cast<JobQueue*>(queue)->retryDlqJob(job_id);
}

void sp_rt_job_queue_clear_dlq(void* queue) {
    if (queue) static_cast<JobQueue*>(queue)->clearDlq();
}

// ============================================================================
// Cron C-ABI
// ============================================================================

void* sp_rt_cron_create() {
    return new CronScheduler();
}

void sp_rt_cron_destroy(void* cron) {
    if (cron) {
        delete static_cast<CronScheduler*>(cron);
    }
}

void* sp_rt_cron_default() {
    return &CronScheduler::getGlobal();
}

void sp_rt_cron_schedule_interval(void* cron, const char* name, int32_t interval_ms, void (*task_fn)()) {
    if (!cron || !name || !task_fn) return;
    auto* c = static_cast<CronScheduler*>(cron);
    c->scheduleInterval(name, interval_ms, [task_fn]() { task_fn(); });
}

void sp_rt_cron_schedule(void* cron, const char* name, const char* cron_expr, void (*task_fn)()) {
    if (!cron || !name || !cron_expr || !task_fn) return;
    auto* c = static_cast<CronScheduler*>(cron);
    c->scheduleCron(name, cron_expr, [task_fn]() { task_fn(); });
}

void sp_rt_cron_start(void* cron) {
    if (cron) static_cast<CronScheduler*>(cron)->start();
}

void sp_rt_cron_stop(void* cron) {
    if (cron) static_cast<CronScheduler*>(cron)->stop();
}

int32_t sp_rt_cron_task_count(void* cron) {
    if (!cron) return 0;
    return static_cast<int32_t>(static_cast<CronScheduler*>(cron)->taskCount());
}

} // extern "C"
