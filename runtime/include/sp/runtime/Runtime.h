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

    void*       sp_rt_http_resp_create(int64_t status, const char* body);
    void        sp_rt_http_resp_set_status(void* resp, int64_t status);
    void        sp_rt_http_resp_set_header(void* resp, const char* name, const char* val);
    void        sp_rt_http_resp_set_cookie(void* resp, const char* name, const char* val, 
                                           const char* path, int64_t max_age, 
                                           bool secure, bool http_only, const char* same_site);
    void        sp_rt_http_resp_set_body(void* resp, const char* body);
    void        sp_rt_http_resp_set_json(void* resp, const char* json_str);

    void*       sp_rt_http_server_create();
    void*       sp_rt_http_server_create_advanced(int64_t workers, int64_t max_body_bytes, int64_t max_header_bytes);
    void        sp_rt_http_server_route(void* srv, const char* method, const char* path, 
                                        const char* (*handler_fn)(const char* req_method, const char* req_path, const char* req_body));
    void        sp_rt_http_server_route_handler(void* srv, const char* method, const char* path, 
                                                void* (*handler_fn)(void* req));
    bool        sp_rt_http_server_listen(void* srv, const char* host, int64_t port);
    int64_t     sp_rt_http_server_port(void* srv);
    void        sp_rt_http_server_stop(void* srv);
    void        sp_rt_http_server_destroy(void* srv);
}

