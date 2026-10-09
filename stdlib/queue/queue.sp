module std.queue;

import std.io;

// ============================================================================
// Standard Library Background Workers, Job Queue & Cron (std.queue) (SP 2.9)
// Provides a thread-safe asynchronous job queue, worker pool, exponential
// backoff retries, Dead-Letter Queue (DLQ), and periodic Cron scheduler.
// ============================================================================

extern "C" {
    fn sp_rt_job_queue_create(int worker_count) -> ptr;
    fn sp_rt_job_queue_destroy(ptr queue);
    fn sp_rt_job_queue_default() -> ptr;
    fn sp_rt_job_queue_start(ptr queue);
    fn sp_rt_job_queue_stop(ptr queue);
    fn sp_rt_job_queue_wait_idle(ptr queue);
    fn sp_rt_job_queue_enqueue(ptr queue, string name, string payload, int max_retries, int backoff_ms, int delay_ms) -> string;
    fn sp_rt_job_queue_pending_count(ptr queue) -> int;
    fn sp_rt_job_queue_active_count(ptr queue) -> int;
    fn sp_rt_job_queue_completed_count(ptr queue) -> int;
    fn sp_rt_job_queue_dlq_count(ptr queue) -> int;
    fn sp_rt_job_queue_dlq_get(ptr queue, int index) -> string;
    fn sp_rt_job_queue_retry_dlq(ptr queue, string job_id) -> bool;
    fn sp_rt_job_queue_clear_dlq(ptr queue);

    fn sp_rt_cron_create() -> ptr;
    fn sp_rt_cron_destroy(ptr cron);
    fn sp_rt_cron_default() -> ptr;
    fn sp_rt_cron_start(ptr cron);
    fn sp_rt_cron_stop(ptr cron);
    fn sp_rt_cron_task_count(ptr cron) -> int;
}

public class JobQueue {
    public ptr handle;

    public JobQueue(int workerCount = 2) {
        this.handle = sp_rt_job_queue_create(workerCount);
    }

    public static fn create(int workerCount = 2) -> JobQueue {
        return new JobQueue(workerCount);
    }

    public static fn getGlobal() -> JobQueue {
        let q = new JobQueue();
        if (q.handle != null) {
            sp_rt_job_queue_destroy(q.handle);
        }
        q.handle = sp_rt_job_queue_default();
        return q;
    }

    public fn start() {
        if (this.handle != null) {
            sp_rt_job_queue_start(this.handle);
        }
    }

    public fn stop() {
        if (this.handle != null) {
            sp_rt_job_queue_stop(this.handle);
        }
    }

    public fn waitIdle() {
        if (this.handle != null) {
            sp_rt_job_queue_wait_idle(this.handle);
        }
    }

    public fn enqueue(string name, string payload, int maxRetries = 3, int backoffMs = 100, int delayMs = 0) -> string {
        if (this.handle == null) { return ""; }
        return sp_rt_job_queue_enqueue(this.handle, name, payload, maxRetries, backoffMs, delayMs);
    }

    public fn pendingCount() -> int {
        if (this.handle == null) { return 0; }
        return sp_rt_job_queue_pending_count(this.handle);
    }

    public fn activeCount() -> int {
        if (this.handle == null) { return 0; }
        return sp_rt_job_queue_active_count(this.handle);
    }

    public fn completedCount() -> int {
        if (this.handle == null) { return 0; }
        return sp_rt_job_queue_completed_count(this.handle);
    }

    public fn dlqCount() -> int {
        if (this.handle == null) { return 0; }
        return sp_rt_job_queue_dlq_count(this.handle);
    }

    public fn dlqGet(int index) -> string {
        if (this.handle == null) { return "{}"; }
        return sp_rt_job_queue_dlq_get(this.handle, index);
    }

    public fn retryDlq(string jobId) -> bool {
        if (this.handle == null) { return false; }
        return sp_rt_job_queue_retry_dlq(this.handle, jobId);
    }

    public fn clearDlq() {
        if (this.handle != null) {
            sp_rt_job_queue_clear_dlq(this.handle);
        }
    }
}

public class CronScheduler {
    public ptr handle;

    public CronScheduler() {
        this.handle = sp_rt_cron_create();
    }

    public static fn create() -> CronScheduler {
        return new CronScheduler();
    }

    public static fn getGlobal() -> CronScheduler {
        let c = new CronScheduler();
        if (c.handle != null) {
            sp_rt_cron_destroy(c.handle);
        }
        c.handle = sp_rt_cron_default();
        return c;
    }

    public fn start() {
        if (this.handle != null) {
            sp_rt_cron_start(this.handle);
        }
    }

    public fn stop() {
        if (this.handle != null) {
            sp_rt_cron_stop(this.handle);
        }
    }

    public fn taskCount() -> int {
        if (this.handle == null) { return 0; }
        return sp_rt_cron_task_count(this.handle);
    }
}

public class Queue {
    public static fn enqueue(string name, string payload, int maxRetries = 3, int backoffMs = 100, int delayMs = 0) -> string {
        return JobQueue.getGlobal().enqueue(name, payload, maxRetries, backoffMs, delayMs);
    }

    public static fn waitIdle() {
        JobQueue.getGlobal().waitIdle();
    }

    public static fn pendingCount() -> int {
        return JobQueue.getGlobal().pendingCount();
    }

    public static fn dlqCount() -> int {
        return JobQueue.getGlobal().dlqCount();
    }

    public static fn retryDlq(string jobId) -> bool {
        return JobQueue.getGlobal().retryDlq(jobId);
    }

    public static fn clearDlq() {
        JobQueue.getGlobal().clearDlq();
    }
}
