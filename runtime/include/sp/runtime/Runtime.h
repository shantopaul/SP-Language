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

    // Phase 11: Memory Diagnostics & Active Tracking
    uint64_t    sp_rt_mem_allocated_bytes();
    uint64_t    sp_rt_mem_active_arc_objects();

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
}
