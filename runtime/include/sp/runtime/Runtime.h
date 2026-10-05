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
    void sp_rt_println_double(double val);
    void sp_rt_println_bool(bool val);

    void sp_rt_print_string(const char* str);
    void sp_rt_print_int(int64_t val);

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
}

