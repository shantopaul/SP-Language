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

    // Diagnostics and assertions
    void sp_rt_assert(bool cond, const char* msg);
    void sp_rt_panic(const char* msg);

    // Object and dynamic memory management
    void* sp_rt_alloc(uint64_t size);
    void sp_rt_free(void* ptr);

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

    // Native Map<K, V> primitives
    void* sp_rt_map_alloc();
    int64_t sp_rt_map_size(void* map);
    void sp_rt_map_set_str(void* map, const char* key, int64_t val);
    int64_t sp_rt_map_get_str(void* map, const char* key);
    bool sp_rt_map_contains_str(void* map, const char* key);
    void sp_rt_map_set_i64(void* map, int64_t key, int64_t val);
    int64_t sp_rt_map_get_i64(void* map, int64_t key);

    // Native Set<T> primitives
    void* sp_rt_set_alloc();
    int64_t sp_rt_set_size(void* set);
    void sp_rt_set_add_i64(void* set, int64_t val);
    void sp_rt_set_add_str(void* set, const char* val);
    bool sp_rt_set_contains_i64(void* set, int64_t val);
    bool sp_rt_set_contains_str(void* set, const char* val);
    void sp_rt_set_remove_i64(void* set, int64_t val);
    void sp_rt_set_remove_str(void* set, const char* val);

    // Higher-order collection functions
    void* sp_rt_array_map(void* arr, int64_t (*fn)(int64_t));
    void* sp_rt_array_filter(void* arr, int64_t (*predicate)(int64_t));
    int64_t sp_rt_array_reduce(void* arr, int64_t (*fn)(int64_t, int64_t), int64_t initial);

    void* sp_rt_list_map(void* list, int64_t (*fn)(int64_t));
    void* sp_rt_list_filter(void* list, int64_t (*predicate)(int64_t));
    int64_t sp_rt_list_reduce(void* list, int64_t (*fn)(int64_t, int64_t), int64_t initial);
}

