#pragma once

#include <cstdint>
#include <string_view>

extern "C" {
    // Core runtime initialization & shutdown
    void sp_runtime_init();
    void sp_runtime_shutdown();

    // Standard I/O built-in runtime functions
    void sp_rt_println_string(const char* str);
    void sp_rt_println_int(int64_t val);
    void sp_rt_println_uint(uint64_t val);
    void sp_rt_println_float(float val);
    void sp_rt_println_double(double val);
    void sp_rt_println_bool(bool val);
    void sp_rt_println_char(char val);

    void sp_rt_print_string(const char* str);
    void sp_rt_print_int(int64_t val);
    void sp_rt_print_uint(uint64_t val);
    void sp_rt_print_float(float val);
    void sp_rt_print_double(double val);
    void sp_rt_print_bool(bool val);
    void sp_rt_print_char(char val);

    const char* sp_rt_read_line();
    const char* sp_rt_readln();

    void sp_rt_eprint_string(const char* str);
    void sp_rt_eprintln_string(const char* str);
    void sp_rt_flush();
    void sp_rt_eflush();

    int64_t     sp_rt_str_to_int(const char* s);
    double      sp_rt_str_to_double(const char* s);
    float       sp_rt_str_to_float(const char* s);
    bool        sp_rt_str_to_bool(const char* s);

    // String operations
    const char* sp_rt_str_concat(const char* s1, const char* s2);
    bool        sp_rt_str_equals(const char* s1, const char* s2);
    const char* sp_rt_str_from_int(int64_t val);
    const char* sp_rt_str_from_uint(uint64_t val);
    const char* sp_rt_str_from_float(float val);
    const char* sp_rt_str_from_double(double val);
    const char* sp_rt_str_from_bool(bool val);
    const char* sp_rt_str_from_char(char val);
    int64_t     sp_rt_str_len(const char* s);
    const char* sp_rt_str_upper(const char* s);
    const char* sp_rt_str_lower(const char* s);
    const char* sp_rt_str_trim(const char* s);
    bool        sp_rt_str_contains(const char* s, const char* sub);
    bool        sp_rt_str_starts_with(const char* s, const char* prefix);
    bool        sp_rt_str_ends_with(const char* s, const char* suffix);
    int64_t     sp_rt_str_index_of(const char* s, const char* sub);
    const char* sp_rt_str_substring(const char* s, int64_t start, int64_t end);
    const char* sp_rt_str_replace(const char* s, const char* from, const char* to);
    char        sp_rt_str_char_at(const char* s, int64_t idx);
    void*       sp_rt_str_split(const char* s, const char* delim);
    const char* sp_rt_str_reverse(const char* s);
    const char* sp_rt_str_repeat(const char* s, int64_t count);
    bool        sp_rt_str_is_empty(const char* s);

    // Diagnostics and assertions
    void sp_rt_assert(bool cond, const char* msg);
    void sp_rt_panic(const char* msg);

    // Object and dynamic memory management
    void* sp_rt_alloc(uint64_t size);
    void  sp_rt_free(void* ptr);

    // Phase 11: Deterministic ARC and Object Lifecycle Management
    typedef void (*SPDestructorFn)(void*);

    void*       sp_rt_arc_alloc(uint64_t size, SPDestructorFn dtor);
    void*       sp_rt_retain(void* ptr);
    void        sp_rt_release(void* ptr);
    uint32_t    sp_rt_strong_count(void* ptr);
    uint32_t    sp_rt_weak_count(void* ptr);

    // Phase 11: Weak References (Cycle-breaking)
    void*       sp_rt_weak_create(void* strong_ptr);
    void*       sp_rt_weak_retain(void* weak_ptr);
    void        sp_rt_weak_release(void* weak_ptr);
    void*       sp_rt_weak_upgrade(void* weak_ptr);

    // Phase 11: Memory Arena Subsystem (std.mem.Arena)
    void*       sp_rt_arena_create(uint64_t initial_capacity);
    void*       sp_rt_arena_alloc(void* arena, uint64_t size, uint64_t alignment);
    void        sp_rt_arena_reset(void* arena);
    void        sp_rt_arena_destroy(void* arena);
    uint64_t    sp_rt_arena_allocated(void* arena);
    uint64_t    sp_rt_arena_allocated_bytes(void* arena);

    // Phase 11: Memory Diagnostics & Active Tracking
    uint64_t    sp_rt_mem_allocated_bytes();
    uint64_t    sp_rt_mem_active_arc_objects();
    uint64_t    sp_rt_mem_live_objects();

    // Native Array primitives
    void* sp_rt_array_alloc(int64_t length, int64_t element_size);
    int64_t sp_rt_array_length(void* arr);
    void* sp_rt_array_data(void* arr);
    void* sp_rt_array_get_ptr(void* arr, int64_t index, int64_t element_size);
    int64_t sp_rt_array_get_i64(void* arr, int64_t index);
    void sp_rt_array_set_i64(void* arr, int64_t index, int64_t val);
    double sp_rt_array_get_f64(void* arr, int64_t index);
    void sp_rt_array_set_f64(void* arr, int64_t index, double val);

    // Native List<T> primitives
    void* sp_rt_list_alloc(int64_t element_size, int64_t initial_capacity);
    int64_t sp_rt_list_size(void* list);
    void sp_rt_list_push_i64(void* list, int64_t val);
    int64_t sp_rt_list_get_i64(void* list, int64_t index);
    void sp_rt_list_set_i64(void* list, int64_t index, int64_t val);
    int64_t sp_rt_list_pop_i64(void* list);
    void sp_rt_list_clear(void* list);
    bool sp_rt_list_contains_i64(void* list, int64_t val);
    void sp_rt_list_remove_at(void* list, int64_t index);
    bool sp_rt_list_is_empty(void* list);

    // Native Map<K, V> primitives
    void* sp_rt_map_alloc();
    int64_t sp_rt_map_size(void* map);
    void sp_rt_map_set_str(void* map, const char* key, int64_t val);
    int64_t sp_rt_map_get_str(void* map, const char* key);
    bool sp_rt_map_contains_str(void* map, const char* key);
    // Phase 6 slice A: key access by ordinal for `for (key in map)`.
    const char* sp_rt_map_str_key_at(void* map, int64_t index);
    int64_t sp_rt_map_int_key_at(void* map, int64_t index);
    void sp_rt_map_set_i64(void* map, int64_t key, int64_t val);
    int64_t sp_rt_map_get_i64(void* map, int64_t key);
    bool sp_rt_map_contains_i64(void* map, int64_t key);
    void sp_rt_map_remove_str(void* map, const char* key);
    void sp_rt_map_remove_i64(void* map, int64_t key);
    void sp_rt_map_clear(void* map);
    bool sp_rt_map_is_empty(void* map);

    // Native Set<T> primitives
    void* sp_rt_set_alloc();
    int64_t sp_rt_set_size(void* set);
    void sp_rt_set_add_i64(void* set, int64_t val);
    void sp_rt_set_add_str(void* set, const char* val);
    bool sp_rt_set_contains_i64(void* set, int64_t val);
    bool sp_rt_set_contains_str(void* set, const char* val);
    void sp_rt_set_remove_i64(void* set, int64_t val);
    void sp_rt_set_remove_str(void* set, const char* val);
    void sp_rt_set_clear(void* set);
    bool sp_rt_set_is_empty(void* set);
    // Phase 6 slice B: indexed access (for-in) + printable form.
    int64_t sp_rt_set_get_i64(void* set, int64_t index);
    const char* sp_rt_set_get_str(void* set, int64_t index);
    const char* sp_rt_set_to_string(void* set);

    bool sp_rt_array_is_empty(void* arr);

    // Native Stack<T> primitives
    void* sp_rt_stack_alloc();
    void sp_rt_stack_push(void* stack, int64_t val);
    int64_t sp_rt_stack_pop(void* stack);
    int64_t sp_rt_stack_peek(void* stack);
    int64_t sp_rt_stack_size(void* stack);
    bool sp_rt_stack_is_empty(void* stack);
    void sp_rt_stack_clear(void* stack);

    // Native Queue<T> primitives
    void* sp_rt_queue_alloc();
    void sp_rt_queue_enqueue(void* queue, int64_t val);
    int64_t sp_rt_queue_dequeue(void* queue);
    int64_t sp_rt_queue_peek(void* queue);
    int64_t sp_rt_queue_size(void* queue);
    bool sp_rt_queue_is_empty(void* queue);
    void sp_rt_queue_clear(void* queue);

    // Native PriorityQueue<T> primitives
    void* sp_rt_pq_alloc();
    void sp_rt_pq_push(void* pq, int64_t val, int64_t priority);
    int64_t sp_rt_pq_pop(void* pq);
    int64_t sp_rt_pq_peek(void* pq);
    int64_t sp_rt_pq_peek_priority(void* pq);
    int64_t sp_rt_pq_size(void* pq);
    bool sp_rt_pq_is_empty(void* pq);
    void sp_rt_pq_clear(void* pq);

    // Higher-order collection functions
    void* sp_rt_array_map(void* arr, int64_t (*fn)(int64_t));
    void* sp_rt_array_filter(void* arr, int64_t (*predicate)(int64_t));
    int64_t sp_rt_array_reduce(void* arr, int64_t (*fn)(int64_t, int64_t), int64_t initial);

    void* sp_rt_list_map(void* list, int64_t (*fn)(int64_t));
    void* sp_rt_list_filter(void* list, int64_t (*predicate)(int64_t));
    int64_t sp_rt_list_reduce(void* list, int64_t (*fn)(int64_t, int64_t), int64_t initial);

    // Math functions
    double sp_rt_math_sqrt(double x);
    double sp_rt_math_pow(double base, double exp);
    double sp_rt_math_sin(double x);
    double sp_rt_math_cos(double x);
    double sp_rt_math_tan(double x);
    double sp_rt_math_asin(double x);
    double sp_rt_math_acos(double x);
    double sp_rt_math_atan(double x);
    double sp_rt_math_atan2(double y, double x);
    double sp_rt_math_floor(double x);
    double sp_rt_math_ceil(double x);
    double sp_rt_math_round(double x);
    double sp_rt_math_abs_double(double x);
    int64_t sp_rt_math_abs_int(int64_t x);
    double sp_rt_math_min_double(double a, double b);
    int64_t sp_rt_math_min_int(int64_t a, int64_t b);
    double sp_rt_math_max_double(double a, double b);
    int64_t sp_rt_math_max_int(int64_t a, int64_t b);
    double sp_rt_math_log(double x);
    double sp_rt_math_log10(double x);
    double sp_rt_math_log2(double x);
    double sp_rt_math_exp(double x);

    // File system functions (std.fs)
    const char* sp_rt_fs_read_text(const char* path);
    bool        sp_rt_fs_write_text(const char* path, const char* content);
    bool        sp_rt_fs_append_text(const char* path, const char* content);
    bool        sp_rt_fs_exists(const char* path);
    bool        sp_rt_fs_delete(const char* path);
    bool        sp_rt_fs_create_dir(const char* path);
    void*       sp_rt_fs_list_dir(const char* path);
    bool        sp_rt_fs_is_file(const char* path);
    bool        sp_rt_fs_is_dir(const char* path);

    // Time functions (std.time)
    int64_t     sp_rt_time_now_ms();
    int64_t     sp_rt_time_now_secs();
    int64_t     sp_rt_time_now_ns();
    void        sp_rt_time_sleep_ms(int64_t ms);
    void        sp_rt_time_sleep_secs(int64_t s);
    const char* sp_rt_time_format_iso(int64_t epoch_ms);
    int64_t     sp_rt_time_parse_iso(const char* iso_str);

    // Duration functions (std.time)
    void*       sp_rt_duration_from_millis(int64_t ms);
    void*       sp_rt_duration_from_seconds(int64_t s);
    void*       sp_rt_duration_from_minutes(int64_t m);
    void*       sp_rt_duration_from_hours(int64_t h);
    int64_t     sp_rt_duration_to_millis(void* d);
    int64_t     sp_rt_duration_to_seconds(void* d);

    // OS functions (std.os)
    const char* sp_rt_os_get_env(const char* name);
    bool        sp_rt_os_set_env(const char* name, const char* val);
    const char* sp_rt_os_platform();
    const char* sp_rt_os_arch();
    void*       sp_rt_os_args();
    // Phase 6 slice B: the generated `main` calls this first so
    // sp_rt_os_args() can return the real argv (argv[1..]).
    void        sp_rt_set_args(int argc, char** argv);
    void        sp_rt_os_exit(int64_t code);

    // Process functions (std.process)
    int64_t     sp_rt_process_exec(const char* cmd);
    int64_t     sp_rt_process_spawn(const char* cmd);
    const char* sp_rt_process_run(const char* cmd);
    int64_t     sp_rt_process_pid();

    // ========================================================================
    // Phase 15: Concurrency, Thread Pool & Lock-Free Channels (std.sync)
    // ========================================================================

    // OS Threads
    int64_t     sp_rt_thread_spawn(void (*fn)(void*), void* arg);
    void        sp_rt_thread_join(int64_t handle);
    void        sp_rt_thread_detach(int64_t handle);
    int64_t     sp_rt_thread_id();
    int64_t     sp_rt_thread_hardware_concurrency();
    void        sp_rt_thread_yield();
    void        sp_rt_thread_sleep_ms(int64_t ms);

    // Mutex
    void*       sp_rt_mutex_create();
    void        sp_rt_mutex_lock(void* mtx);
    bool        sp_rt_mutex_try_lock(void* mtx);
    void        sp_rt_mutex_unlock(void* mtx);
    void        sp_rt_mutex_destroy(void* mtx);

    // RwLock
    void*       sp_rt_rwlock_create();
    void        sp_rt_rwlock_read_lock(void* rw);
    bool        sp_rt_rwlock_try_read_lock(void* rw);
    void        sp_rt_rwlock_read_unlock(void* rw);
    void        sp_rt_rwlock_write_lock(void* rw);
    bool        sp_rt_rwlock_try_write_lock(void* rw);
    void        sp_rt_rwlock_write_unlock(void* rw);
    void        sp_rt_rwlock_destroy(void* rw);

    // Semaphore
    void*       sp_rt_semaphore_create(int64_t initial_count);
    void        sp_rt_semaphore_acquire(void* sem);
    bool        sp_rt_semaphore_try_acquire(void* sem);
    void        sp_rt_semaphore_release(void* sem);
    void        sp_rt_semaphore_destroy(void* sem);

    // ConditionVariable
    void*       sp_rt_condvar_create();
    void        sp_rt_condvar_wait(void* cv, void* mtx);
    bool        sp_rt_condvar_wait_timeout(void* cv, void* mtx, int64_t timeout_ms);
    void        sp_rt_condvar_notify_one(void* cv);
    void        sp_rt_condvar_notify_all(void* cv);
    void        sp_rt_condvar_destroy(void* cv);

    // Atomics: AtomicInt
    void*       sp_rt_atomic_int_create(int64_t initial_val);
    int64_t     sp_rt_atomic_int_load(void* atom);
    void        sp_rt_atomic_int_store(void* atom, int64_t val);
    int64_t     sp_rt_atomic_int_add(void* atom, int64_t val);
    int64_t     sp_rt_atomic_int_sub(void* atom, int64_t val);
    int64_t     sp_rt_atomic_int_exchange(void* atom, int64_t val);
    bool        sp_rt_atomic_int_cas(void* atom, int64_t expected, int64_t desired);
    void        sp_rt_atomic_int_destroy(void* atom);

    // Atomics: AtomicBool
    void*       sp_rt_atomic_bool_create(bool initial_val);
    bool        sp_rt_atomic_bool_load(void* atom);
    void        sp_rt_atomic_bool_store(void* atom, bool val);
    bool        sp_rt_atomic_bool_exchange(void* atom, bool val);
    bool        sp_rt_atomic_bool_cas(void* atom, bool expected, bool desired);
    void        sp_rt_atomic_bool_destroy(void* atom);

    // Atomics: AtomicPtr
    void*       sp_rt_atomic_ptr_create(void* initial_val);
    void*       sp_rt_atomic_ptr_load(void* atom);
    void        sp_rt_atomic_ptr_store(void* atom, void* val);
    void*       sp_rt_atomic_ptr_exchange(void* atom, void* val);
    bool        sp_rt_atomic_ptr_cas(void* atom, void* expected, void* desired);
    void        sp_rt_atomic_ptr_destroy(void* atom);

    // Channel
    void*       sp_rt_channel_create(int64_t capacity);
    bool        sp_rt_channel_send(void* chan, void* item);
    void*       sp_rt_channel_recv(void* chan);
    bool        sp_rt_channel_try_send(void* chan, void* item);
    bool        sp_rt_channel_try_recv(void* chan, void** out_item);
    bool        sp_rt_channel_send_i64(void* chan, int64_t val);
    int64_t     sp_rt_channel_recv_i64(void* chan);
    bool        sp_rt_channel_try_send_i64(void* chan, int64_t val);
    bool        sp_rt_channel_try_recv_i64(void* chan, int64_t* out_val);
    void        sp_rt_channel_close(void* chan);
    bool        sp_rt_channel_is_closed(void* chan);
    int64_t     sp_rt_channel_size(void* chan);
    int64_t     sp_rt_channel_capacity(void* chan);
    void        sp_rt_channel_destroy(void* chan);

    // ThreadPool
    void*       sp_rt_threadpool_create(int64_t num_threads);
    void        sp_rt_threadpool_submit(void* pool, void (*fn)(void*), void* arg);
    void        sp_rt_threadpool_wait_idle(void* pool);
    int64_t     sp_rt_threadpool_thread_count(void* pool);
    void        sp_rt_threadpool_destroy(void* pool);

    // ========================================================================
    // Phase 16: Non-Blocking Networking, JSON & HTTP Engine (SP 1.6)
    // ========================================================================

    // Network
    void        sp_rt_net_init();
    void        sp_rt_net_shutdown();
    const char* sp_rt_net_resolve(const char* host);

    // TCP Client
    void*       sp_rt_tcp_connect(const char* host, int64_t port, int64_t timeout_ms);
    int64_t     sp_rt_tcp_write(void* sock, const char* str);
    const char* sp_rt_tcp_read(void* sock, int64_t max_bytes);
    const char* sp_rt_tcp_read_all(void* sock, int64_t timeout_ms);
    bool        sp_rt_tcp_set_nonblocking(void* sock, bool nb);
    bool        sp_rt_tcp_set_timeout(void* sock, int64_t ms);
    void        sp_rt_tcp_close(void* sock);

    // TCP Listener
    void*       sp_rt_tcp_listen(const char* host, int64_t port, int64_t backlog);
    void*       sp_rt_tcp_accept(void* listener);
    void*       sp_rt_tcp_accept_timeout(void* listener, int64_t timeout_ms);
    int64_t     sp_rt_tcp_listener_port(void* listener);
    void        sp_rt_tcp_listener_close(void* listener);

    // UDP Socket
    void*       sp_rt_udp_bind(const char* host, int64_t port);
    int64_t     sp_rt_udp_send_to(void* sock, const char* data, const char* host, int64_t port);
    const char* sp_rt_udp_recv_from(void* sock, int64_t max_len, const char** out_sender_host, int64_t* out_sender_port);
    void        sp_rt_udp_close(void* sock);

    // EventLoop
    void*       sp_rt_eventloop_create();
    bool        sp_rt_eventloop_add(void* loop, void* sock, int64_t events);
    bool        sp_rt_eventloop_remove(void* loop, void* sock);
    int64_t     sp_rt_eventloop_poll(void* loop, int64_t timeout_ms);
    void        sp_rt_eventloop_destroy(void* loop);

    // Async (Phase 2 slice B2): timer scheduler + Task futures. The
    // scheduler handle is the process-wide default scheduler; tasks are
    // heap-allocated and owned by the caller until sp_rt_async_task_destroy.
    // Awaiting pumps the scheduler (it does NOT block the loop): other
    // timers/tasks fire while the await is outstanding.
    void*       sp_rt_async_default_scheduler();
    void*       sp_rt_async_sleep_ms(void* sched, int64_t ms);
    void*       sp_rt_async_delayed_i64(void* sched, int64_t ms, int64_t value);
    void*       sp_rt_async_delayed_str(void* sched, int64_t ms, const char* value);
    void        sp_rt_async_task_await_void(void* sched, void* task);
    int64_t     sp_rt_async_task_await_i64(void* sched, void* task);
    const char* sp_rt_async_task_await_str(void* sched, void* task);
    bool        sp_rt_async_task_is_ready(void* task);
    bool        sp_rt_async_task_cancel(void* task);
    bool        sp_rt_async_task_was_cancelled(void* task);
    void        sp_rt_async_task_destroy(void* task);
    void        sp_rt_async_scheduler_run_once(void* sched);
    bool        sp_rt_async_scheduler_has_pending(void* sched);
    void        sp_rt_async_scheduler_run_until_idle(void* sched);
    // Phase 3 slice C: spawn an async body as a task. `thunk` is a
    // compiler-generated `int64_t/const char*/void (*)(void*)`; `ctx` is
    // the heap argument block the thunk loads from and frees. The task
    // is created by sp_rt_async_create_task and owned by the caller.
    void*       sp_rt_async_create_task();
    void        sp_rt_async_spawn_i64(void* sched, void* task, void* thunk, void* ctx);
    void        sp_rt_async_spawn_str(void* sched, void* task, void* thunk, void* ctx);
    void        sp_rt_async_spawn_void(void* sched, void* task, void* thunk, void* ctx);
    // Phase 5 slice A (state machine): spawn a state-machine async body.
    // `thunk` is the compiler-generated body entry `void (*)(void* task,
    // void* ctx)`; it allocates the heap frame, stores task + args, and
    // calls the resume function. Unlike the spawn_* fns above, this one
    // does NOT complete the task — the body completes it in its finish
    // block. A task cancelled before the posted callback runs never runs
    // its body at all (fixes the old spawn fns' documented gap).
    void        sp_rt_async_spawn_sm(void* sched, void* task, void* thunk, void* ctx);
    // Double payload (Task<double>): delayed value + pump-based await.
    void*       sp_rt_async_delayed_dbl(void* sched, int64_t ms, double value);
    double      sp_rt_async_task_await_dbl(void* sched, void* task);
    // Pointer payload (Task<T[]> / Task<struct>): pump-based await.
    void*       sp_rt_async_task_await_ptr(void* sched, void* task);
    // Non-blocking value reads (for state-machine continuations: the
    // awaited task is already complete). Return the default value
    // (0 / "" / 0.0 / null) when the task was cancelled or has no such
    // payload. The str reader returns a fresh owned copy.
    int64_t     sp_rt_async_task_value_i64(void* task);
    const char* sp_rt_async_task_value_str(void* task);
    double      sp_rt_async_task_value_dbl(void* task);
    void*       sp_rt_async_task_value_ptr(void* task);
    // Complete a task from generated code (the async body finish block).
    // No-op when the task is already complete/cancelled.
    void        sp_rt_async_task_complete_i64(void* task, int64_t value);
    void        sp_rt_async_task_complete_str(void* task, const char* value);
    void        sp_rt_async_task_complete_dbl(void* task, double value);
    void        sp_rt_async_task_complete_ptr(void* task, void* value);
    void        sp_rt_async_task_complete_ptr_owned(void* task, void* value);
    void        sp_rt_async_task_complete_void(void* task);
    // Register a resume for a state-machine await: when `task` completes
    // or is cancelled, `resumeFn(frame)` is POSTED to the scheduler
    // (never called synchronously, so no re-entrant resume). `resumeFn`
    // is the compiler-generated `void (*)(void* frame)` resume function.
    // Phase 6 slice C: `ownTask` is the awaiting task — the resume is
    // registered on it too, so a cancel while suspended resumes the
    // body into its continuation (which frees the frame) instead of
    // leaking it. Either may be null (only `resumeFn` is required).
    void        sp_rt_async_task_on_complete_resume(void* sched, void* task, void* resumeFn, void* frame, void* ownTask);
    // Combinators: `tasks` is the SP array object (a Task<T>[] value)
    // holding `count` task handles; the element storage pointer is
    // extracted inside via sp_rt_array_data. Returns a new task handle
    // owned by the caller. all_* waits for every input (result: SP
    // array / void); race_* completes with the first finisher. See
    // Async.h for semantics.
    void*       sp_rt_async_task_all_i64(void* tasks, int64_t count);
    void*       sp_rt_async_task_all_str(void* tasks, int64_t count);
    void*       sp_rt_async_task_all_dbl(void* tasks, int64_t count);
    void*       sp_rt_async_task_all_void(void* tasks, int64_t count);
    void*       sp_rt_async_task_race_i64(void* tasks, int64_t count);
    void*       sp_rt_async_task_race_str(void* tasks, int64_t count);
    void*       sp_rt_async_task_race_dbl(void* tasks, int64_t count);
    void*       sp_rt_async_task_race_void(void* tasks, int64_t count);
    // Drain timers/queued tasks only (registered sockets do NOT keep the
    // drain going): the end-of-main pump for fire-and-forget tasks.
    void        sp_rt_async_scheduler_run_tasks_until_idle(void* sched);

    // Crypto
    const char* sp_rt_crypto_sha256(const char* data);
    const char* sp_rt_crypto_hmac_sha256(const char* key, const char* data);
    const char* sp_rt_crypto_base64_encode(const char* data);
    const char* sp_rt_crypto_base64_decode(const char* data);
    const char* sp_rt_crypto_random_hex(int64_t bytes);
    const char* sp_rt_crypto_uuid();

    // JSON
    void*       sp_rt_json_parse(const char* json_str);
    const char* sp_rt_json_stringify(void* handle, int64_t indent);
    int64_t     sp_rt_json_get_type(void* handle);
    bool        sp_rt_json_get_bool(void* handle);
    int64_t     sp_rt_json_get_int(void* handle);
    double      sp_rt_json_get_double(void* handle);
    const char* sp_rt_json_get_string(void* handle);
    int64_t     sp_rt_json_arr_size(void* handle);
    void*       sp_rt_json_arr_get(void* handle, int64_t idx);
    void        sp_rt_json_arr_add(void* handle, void* item_handle);
    void*       sp_rt_json_obj_get(void* handle, const char* key);
    void        sp_rt_json_obj_set(void* handle, const char* key, void* item_handle);
    bool        sp_rt_json_obj_has(void* handle, const char* key);
    void*       sp_rt_json_new_null();
    void*       sp_rt_json_new_bool(bool b);
    void*       sp_rt_json_new_int(int64_t i);
    void*       sp_rt_json_new_double(double d);
    void*       sp_rt_json_new_string(const char* s);
    void*       sp_rt_json_new_array();
    void*       sp_rt_json_new_object();
    void        sp_rt_json_destroy(void* handle);

    // Phase 22: Typed JSON & Class Metadata Bridges
    void        sp_rt_json_register_class(const char* class_name, uint32_t object_size, bool has_vtable);
    void        sp_rt_json_register_field(const char* class_name, const char* field_name, const char* json_key, 
                                          int32_t type_kind, const char* type_name, uint32_t byte_offset, 
                                          bool is_ignored, bool is_nullable);
    void*       sp_rt_json_decode_typed(const char* class_name, const char* json_str, char** out_err);
    const char* sp_rt_json_encode_typed(const char* class_name, const void* obj_ptr, int64_t indent);
    const char* sp_rt_json_problem_details(const char* title, int32_t status, const char* detail, 
                                           const char* instance, const char* errors_json);

    // HTTP
    void*       sp_rt_http_get(const char* url, int64_t timeout_ms);
    void*       sp_rt_http_post(const char* url, const char* body, const char* content_type, int64_t timeout_ms);
    int64_t     sp_rt_http_resp_status(void* resp);
    const char* sp_rt_http_resp_status_text(void* resp);
    const char* sp_rt_http_resp_body(void* resp);
    const char* sp_rt_http_resp_header(void* resp, const char* name);
    void        sp_rt_http_resp_destroy(void* resp);

    // Phase 20: HTTP/1.1 Engine & Streaming Server enhancements
    const char* sp_rt_http_req_method(void* req);
    const char* sp_rt_http_req_path(void* req);
    const char* sp_rt_http_req_body(void* req);
    const char* sp_rt_http_req_header(void* req, const char* name);
    const char* sp_rt_http_req_query(void* req, const char* name);
    const char* sp_rt_http_req_param(void* req, const char* name);
    const char* sp_rt_http_req_cookie(void* req, const char* name);
    // Phase 6 slice A: normalize a handler-provided request pointer —
    // a wrapped SP HttpRequest object (slot 0 = vtable, slot 1 = raw
    // handle) yields the raw handle; a raw pointer passes through.
    void*       sp_rt_http_req_unwrap(void* req);

    void*       sp_rt_http_resp_create(int64_t status, const char* body);
    void        sp_rt_http_resp_set_status(void* resp, int64_t status);
    void        sp_rt_http_resp_set_header(void* resp, const char* name, const char* val);
    void        sp_rt_http_resp_set_cookie(void* resp, const char* name, const char* val, 
                                           const char* path, int64_t max_age, 
                                           bool secure, bool http_only, const char* same_site);
    void        sp_rt_http_resp_set_body(void* resp, const char* body);
    void        sp_rt_http_resp_set_json(void* resp, const char* json_str);
    void        sp_rt_http_resp_set_html(void* resp, const char* html);
    void        sp_rt_http_resp_set_text(void* resp, const char* text);
    void        sp_rt_http_resp_redirect(void* resp, const char* url, int64_t status);

    void*       sp_rt_http_server_create();
    void*       sp_rt_http_server_create_advanced(int64_t workers, int64_t max_body_bytes, int64_t max_header_bytes);
    void        sp_rt_http_server_route(void* srv, const char* method, const char* path, 
                                        const char* (*handler_fn)(const char* req_method, const char* req_path, const char* req_body));
    void        sp_rt_http_server_route_handler(void* srv, const char* method, const char* path, 
                                                void* (*handler_fn)(void* req));
    // Phase 4 (parent fix): static-JSON route — registers a route that always
    // answers with the given JSON body (200, application/json). Lets SP code
    // serve a fixed document (e.g. App::openApi) without a capturing lambda,
    // which the language deliberately does not support (loud E1000).
    void        sp_rt_http_server_route_json(void* srv, const char* method, const char* path,
                                            const char* json);
    bool        sp_rt_http_server_listen(void* srv, const char* host, int64_t port);
    void        sp_rt_http_server_wait(void* srv);
    int64_t     sp_rt_http_server_port(void* srv);
    void        sp_rt_http_server_stop(void* srv);
    void        sp_rt_http_server_destroy(void* srv);

    // Phase 21: High-Performance Routing, Route Groups & Middleware Pipeline (SP 2.3)
    void*       sp_rt_http_server_group(void* srv, const char* prefix);
    void*       sp_rt_http_group_group(void* grp, const char* prefix);
    void        sp_rt_http_group_route(void* grp, const char* method, const char* path, 
                                       const char* (*handler_fn)(const char* req_method, const char* req_path, const char* req_body));
    void        sp_rt_http_group_route_handler(void* grp, const char* method, const char* path, 
                                               void* (*handler_fn)(void* req));
    void        sp_rt_http_group_destroy(void* grp);

    void        sp_rt_http_server_use_cors(void* srv, const char* origins, const char* methods, const char* headers);
    void        sp_rt_http_server_use_logger(void* srv);
    void        sp_rt_http_server_use_recovery(void* srv);
    void        sp_rt_http_server_use_request_id(void* srv);
    void        sp_rt_http_server_use_rate_limiter(void* srv, int64_t max_reqs, int64_t window_secs);

    // Phase 23: Tier-1 Databases & Connection Pooling (PostgreSQL & SQLite) (SP 2.5)
    // SQLite C-ABI bindings
    void*       sp_rt_sqlite_open(const char* path, int64_t busy_timeout_ms, bool wal_mode);
    void        sp_rt_sqlite_close(void* db);
    int64_t     sp_rt_sqlite_exec(void* db, const char* sql);
    const char* sp_rt_sqlite_errmsg(void* db);
    int64_t     sp_rt_sqlite_last_insert_rowid(void* db);
    int64_t     sp_rt_sqlite_changes(void* db);
    bool        sp_rt_sqlite_begin(void* db);
    bool        sp_rt_sqlite_commit(void* db);
    bool        sp_rt_sqlite_rollback(void* db);

    void*       sp_rt_sqlite_prepare(void* db, const char* sql);
    bool        sp_rt_sqlite_bind_int(void* stmt, int64_t index, int64_t val);
    bool        sp_rt_sqlite_bind_double(void* stmt, int64_t index, double val);
    bool        sp_rt_sqlite_bind_string(void* stmt, int64_t index, const char* val);
    bool        sp_rt_sqlite_bind_null(void* stmt, int64_t index);
    int64_t     sp_rt_sqlite_param_index(void* stmt, const char* name);
    bool        sp_rt_sqlite_bind_named_int(void* stmt, const char* name, int64_t val);
    bool        sp_rt_sqlite_bind_named_double(void* stmt, const char* name, double val);
    bool        sp_rt_sqlite_bind_named_string(void* stmt, const char* name, const char* val);
    bool        sp_rt_sqlite_bind_named_null(void* stmt, const char* name);
    int64_t     sp_rt_sqlite_step(void* stmt); // 100 = Row, 101 = Done
    int64_t     sp_rt_sqlite_col_count(void* stmt);
    const char* sp_rt_sqlite_col_name(void* stmt, int64_t index);
    int64_t     sp_rt_sqlite_col_type(void* stmt, int64_t index);
    int64_t     sp_rt_sqlite_col_int(void* stmt, int64_t index);
    double      sp_rt_sqlite_col_double(void* stmt, int64_t index);
    const char* sp_rt_sqlite_col_string(void* stmt, int64_t index);
    bool        sp_rt_sqlite_col_is_null(void* stmt, int64_t index);
    bool        sp_rt_sqlite_reset(void* stmt);
    bool        sp_rt_sqlite_clear_bindings(void* stmt);
    void        sp_rt_sqlite_finalize(void* stmt);

    // PostgreSQL C-ABI bindings
    void*       sp_rt_pg_connect(const char* conn_str);
    void        sp_rt_pg_close(void* conn);
    bool        sp_rt_pg_is_connected(void* conn);
    const char* sp_rt_pg_last_error(void* conn);
    void*       sp_rt_pg_query(void* conn, const char* sql);
    void*       sp_rt_pg_query_params(void* conn, const char* sql, const char** params, int64_t param_count);
    bool        sp_rt_pg_prepare(void* conn, const char* name, const char* sql);
    void*       sp_rt_pg_exec_prepared(void* conn, const char* name, const char** params, int64_t param_count);
    bool        sp_rt_pg_ping(void* conn);
    bool        sp_rt_pg_begin(void* conn);
    bool        sp_rt_pg_commit(void* conn);
    bool        sp_rt_pg_rollback(void* conn);

    bool        sp_rt_pg_res_is_ok(void* res);
    const char* sp_rt_pg_res_error(void* res);
    const char* sp_rt_pg_res_command_tag(void* res);
    int64_t     sp_rt_pg_res_affected_rows(void* res);
    int64_t     sp_rt_pg_res_row_count(void* res);
    int64_t     sp_rt_pg_res_col_count(void* res);
    const char* sp_rt_pg_res_col_name(void* res, int64_t col);
    const char* sp_rt_pg_res_get_value(void* res, int64_t row, int64_t col);
    bool        sp_rt_pg_res_is_null(void* res, int64_t row, int64_t col);
    void        sp_rt_pg_res_free(void* res);

    // Connection Pool C-ABI bindings
    void*       sp_rt_db_pool_create_sqlite(const char* path, int64_t min_conn, int64_t max_conn, int64_t acquire_timeout_ms);
    void*       sp_rt_db_pool_create_pg(const char* conn_str, int64_t min_conn, int64_t max_conn, int64_t acquire_timeout_ms);
    void*       sp_rt_db_pool_acquire(void* pool);
    void        sp_rt_db_pool_release(void* pool, void* conn);
    void        sp_rt_db_pool_invalidate(void* pool, void* conn);
    void        sp_rt_db_pool_close(void* pool);
    int64_t     sp_rt_db_pool_total(void* pool);
    int64_t     sp_rt_db_pool_available(void* pool);
    int64_t     sp_rt_db_pool_in_use(void* pool);

    // ========================================================================
    // Phase 24: Security, Authentication & Cryptography (SP 2.6)
    // ========================================================================
    // Base64Url & Constant-Time Comparison
    const char* sp_rt_crypto_base64url_encode(const char* data);
    const char* sp_rt_crypto_base64url_decode(const char* data);
    bool        sp_rt_crypto_timing_safe_equal(const char* a, const char* b);

    // Password Hashing (Argon2id & Bcrypt)
    const char* sp_rt_password_hash(const char* password, const char* algorithm, int64_t cost_or_memory, int64_t iterations, int64_t parallelism);
    bool        sp_rt_password_verify(const char* password, const char* hash);

    // JWT (JSON Web Tokens - RFC 7519)
    const char* sp_rt_jwt_sign(const char* payload_json, const char* secret, const char* algorithm, int64_t expires_in_seconds, const char* issuer, const char* audience);
    void*       sp_rt_jwt_verify(const char* token, const char* secret, const char* expected_issuer, const char* expected_audience);
    bool        sp_rt_jwt_res_is_valid(void* res);
    const char* sp_rt_jwt_res_error(void* res);
    const char* sp_rt_jwt_res_payload(void* res);
    const char* sp_rt_jwt_res_claim(void* res, const char* claim_name);
    void        sp_rt_jwt_res_free(void* res);

    // Signed Cookies & Session Management
    const char* sp_rt_cookie_sign(const char* val, const char* secret);
    const char* sp_rt_cookie_unsign(const char* signed_val, const char* secret);
    void*       sp_rt_session_store_create(int64_t default_ttl_secs);
    const char* sp_rt_session_create(void* store, const char* data_json, int64_t ttl_secs);
    const char* sp_rt_session_get(void* store, const char* session_id);
    bool        sp_rt_session_set(void* store, const char* session_id, const char* data_json, int64_t ttl_secs);
    bool        sp_rt_session_destroy(void* store, const char* session_id);
    const char* sp_rt_session_regenerate(void* store, const char* old_session_id, int64_t ttl_secs);
    void        sp_rt_session_store_destroy(void* store);

    // Persistent (SQLite-backed) sessions (Phase 31). open() returns null
    // on failure; last_error() explains why. No in-memory fallback.
    void*       sp_rt_psession_open(const char* path, int64_t default_ttl_secs);
    void        sp_rt_psession_close(void* store);
    const char* sp_rt_psession_last_error(void* store);
    const char* sp_rt_psession_create(void* store, const char* data_json, int64_t ttl_secs);
    const char* sp_rt_psession_get(void* store, const char* session_id);
    bool        sp_rt_psession_set(void* store, const char* session_id, const char* data_json, int64_t ttl_secs);
    bool        sp_rt_psession_destroy(void* store, const char* session_id);
    const char* sp_rt_psession_regenerate(void* store, const char* old_session_id, int64_t ttl_secs);
    int64_t     sp_rt_psession_count(void* store);

    // ========================================================================
    // Phase 26: Real-Time Web & Distributed Caching (WebSocket & Redis) (SP 2.8)
    // ========================================================================
    // WebSocket (RFC 6455)
    const char* sp_rt_ws_accept_key(const char* client_key);
    void*       sp_rt_ws_hub_create();
    void        sp_rt_ws_hub_destroy(void* hub);
    void        sp_rt_ws_hub_broadcast(void* hub, const char* msg);
    void        sp_rt_ws_hub_broadcast_room(void* hub, const char* room, const char* msg);
    void        sp_rt_ws_hub_join(void* hub, int64_t conn_id, const char* room);
    void        sp_rt_ws_hub_leave(void* hub, int64_t conn_id, const char* room);
    int64_t     sp_rt_ws_hub_size(void* hub);
    int64_t     sp_rt_ws_hub_room_size(void* hub, const char* room);

    // WebSocket server transport (Phase 31): text handler per upgrade path.
    void        sp_rt_ws_route(const char* path, void (*handler_fn)(int64_t conn_id, const char* msg));
    bool        sp_rt_ws_send(int64_t conn_id, const char* msg);
    void        sp_rt_ws_broadcast(const char* msg);
    void        sp_rt_ws_close_conn(int64_t conn_id);
    int64_t     sp_rt_ws_connection_count();

    // Server-Sent Events (SSE)
    const char* sp_rt_sse_format(const char* event, const char* data, const char* id, int64_t retry);
    const char* sp_rt_sse_heartbeat();

    // Redis (RESP2 / RESP3)
    void*       sp_rt_redis_create(const char* host, int64_t port);
    void        sp_rt_redis_destroy(void* client);
    bool        sp_rt_redis_connect(void* client);
    const char* sp_rt_redis_ping(void* client);
    const char* sp_rt_redis_get(void* client, const char* key);
    bool        sp_rt_redis_set(void* client, const char* key, const char* val);
    bool        sp_rt_redis_setex(void* client, const char* key, int64_t seconds, const char* val);
    bool        sp_rt_redis_del(void* client, const char* key);
    bool        sp_rt_redis_exists(void* client, const char* key);
    int64_t     sp_rt_redis_ttl(void* client, const char* key);
    int64_t     sp_rt_redis_incr(void* client, const char* key);
    int64_t     sp_rt_redis_decr(void* client, const char* key);
    const char* sp_rt_redis_hget(void* client, const char* key, const char* field);
    bool        sp_rt_redis_hset(void* client, const char* key, const char* field, const char* val);
    bool        sp_rt_redis_hdel(void* client, const char* key, const char* field);
    int64_t     sp_rt_redis_lpush(void* client, const char* key, const char* val);
    int64_t     sp_rt_redis_rpush(void* client, const char* key, const char* val);
    const char* sp_rt_redis_lpop(void* client, const char* key);
    const char* sp_rt_redis_rpop(void* client, const char* key);
    int64_t     sp_rt_redis_llen(void* client, const char* key);
    int64_t     sp_rt_redis_publish(void* client, const char* channel, const char* message);

    // Redis Connection Pool
    void*       sp_rt_redis_pool_create(const char* host, int64_t port, int64_t min_conn, int64_t max_conn);
    void        sp_rt_redis_pool_destroy(void* pool);
    void*       sp_rt_redis_pool_acquire(void* pool);
    void        sp_rt_redis_pool_release(void* pool, void* client);
    int64_t     sp_rt_redis_pool_available(void* pool);
    int64_t     sp_rt_redis_pool_total(void* pool);

    // ========================================================================
    // Phase 27: Observability, Background Workers & Production DevOps (SP 2.9)
    // ========================================================================
    // Structured Logging (std.log)
    void*       sp_rt_logger_create(const char* name, int32_t level);
    void        sp_rt_logger_destroy(void* logger);
    void*       sp_rt_logger_default();
    void        sp_rt_logger_set_level(void* logger, int32_t level);
    int32_t     sp_rt_logger_get_level(void* logger);
    void        sp_rt_logger_add_console(void* logger);
    void        sp_rt_logger_add_file(void* logger, const char* filepath, int32_t max_bytes, int32_t max_backups);
    void*       sp_rt_logger_add_memory(void* logger, int32_t capacity);
    void        sp_rt_logger_log(void* logger, int32_t level, const char* message, const char* fields_json);
    void*       sp_rt_logger_with(void* logger, const char* key, const char* val);
    const char* sp_rt_memory_appender_get(void* appender, int32_t index);
    int32_t     sp_rt_memory_appender_size(void* appender);
    void        sp_rt_memory_appender_clear(void* appender);
    void        sp_rt_memory_appender_destroy(void* appender);

    // Prometheus Exposition Metrics (std.metrics)
    void*       sp_rt_metrics_registry_create();
    void        sp_rt_metrics_registry_destroy(void* reg);
    void*       sp_rt_metrics_registry_default();
    void        sp_rt_metrics_counter_inc(void* reg, const char* name, const char* help, const char* labels_json, double val);
    void        sp_rt_metrics_gauge_set(void* reg, const char* name, const char* help, const char* labels_json, double val);
    void        sp_rt_metrics_gauge_add(void* reg, const char* name, const char* help, const char* labels_json, double val);
    void        sp_rt_metrics_histogram_observe(void* reg, const char* name, const char* help, const char* buckets_csv, const char* labels_json, double val);
    const char* sp_rt_metrics_render_prometheus(void* reg);

    // Health & Readiness Checks (std.health)
    void*       sp_rt_health_registry_create();
    void        sp_rt_health_registry_destroy(void* reg);
    void*       sp_rt_health_registry_default();
    void        sp_rt_health_register_check(void* reg, const char* name, bool (*check_fn)());
    void        sp_rt_health_set_status(void* reg, const char* name, bool is_up, const char* message);
    const char* sp_rt_health_check_liveness(void* reg);
    const char* sp_rt_health_check_readiness(void* reg, bool* out_is_up);

    // Background Job Queue & Workers (std.queue)
    void*       sp_rt_job_queue_create(int32_t worker_count);
    void        sp_rt_job_queue_destroy(void* queue);
    void*       sp_rt_job_queue_default();
    void        sp_rt_job_queue_start(void* queue);
    void        sp_rt_job_queue_stop(void* queue);
    void        sp_rt_job_queue_wait_idle(void* queue);
    const char* sp_rt_job_queue_enqueue(void* queue, const char* name, const char* payload, int32_t max_retries, int32_t backoff_ms, int32_t delay_ms);
    void        sp_rt_job_queue_register_handler(void* queue, const char* name, bool (*handler_fn)(const char*, const char*, const char*));
    int32_t     sp_rt_job_queue_pending_count(void* queue);
    int32_t     sp_rt_job_queue_active_count(void* queue);
    int32_t     sp_rt_job_queue_completed_count(void* queue);
    int32_t     sp_rt_job_queue_dlq_count(void* queue);
    const char* sp_rt_job_queue_dlq_get(void* queue, int32_t index);
    bool        sp_rt_job_queue_retry_dlq(void* queue, const char* job_id);
    void        sp_rt_job_queue_clear_dlq(void* queue);

    // Cron Scheduler (std.queue)
    void*       sp_rt_cron_create();
    void        sp_rt_cron_destroy(void* cron);
    void*       sp_rt_cron_default();
    void        sp_rt_cron_schedule_interval(void* cron, const char* name, int32_t interval_ms, void (*task_fn)());
    void        sp_rt_cron_schedule(void* cron, const char* name, const char* cron_expr, void (*task_fn)());
    void        sp_rt_cron_start(void* cron);
    void        sp_rt_cron_stop(void* cron);
    int32_t     sp_rt_cron_task_count(void* cron);

    // Persistent (SQLite-backed) job queue (Phase 31). open() returns null
    // on failure; last_error() explains why. No in-memory fallback.
    // dequeue() returns "{}" when no job is due, else the job as JSON.
    void*       sp_rt_pqueue_open(const char* path);
    void        sp_rt_pqueue_close(void* queue);
    const char* sp_rt_pqueue_last_error(void* queue);
    const char* sp_rt_pqueue_enqueue(void* queue, const char* name, const char* payload, int32_t max_retries, int32_t backoff_ms, int32_t delay_ms);
    const char* sp_rt_pqueue_dequeue(void* queue);
    bool        sp_rt_pqueue_complete(void* queue, const char* job_id);
    bool        sp_rt_pqueue_fail(void* queue, const char* job_id, const char* error);
    int32_t     sp_rt_pqueue_pending_count(void* queue);
    int32_t     sp_rt_pqueue_dlq_count(void* queue);
    bool        sp_rt_pqueue_retry_dlq(void* queue, const char* job_id);
    void        sp_rt_pqueue_clear_dlq(void* queue);

    // ========================================================================
    // Phase 28: In-Memory HTTP Test Client (std.test.http) & OpenAPI Docs
    // ========================================================================
    void*       sp_rt_test_client_create(void* srv_handle);
    void        sp_rt_test_client_destroy(void* client_handle);
    void*       sp_rt_test_client_request(void* client_handle, const char* method, const char* path, const char* body, const char* headers_json);
    void*       sp_rt_test_client_get(void* client_handle, const char* path);
    void*       sp_rt_test_client_post(void* client_handle, const char* path, const char* body);
    void*       sp_rt_test_client_put(void* client_handle, const char* path, const char* body);
    void*       sp_rt_test_client_delete(void* client_handle, const char* path);
    void*       sp_rt_test_client_patch(void* client_handle, const char* path, const char* body);
    int32_t     sp_rt_test_response_status(void* resp_handle);
    const char* sp_rt_test_response_body(void* resp_handle);
    const char* sp_rt_test_response_header(void* resp_handle, const char* name);
    int32_t     sp_rt_test_response_has_header(void* resp_handle, const char* name);
    void        sp_rt_test_response_destroy(void* resp_handle);

    // OpenAPI & Swagger UI / Redoc HTML
    const char* sp_rt_openapi_swagger_html(const char* title, const char* spec_url);
    const char* sp_rt_openapi_redoc_html(const char* title, const char* spec_url);
}


