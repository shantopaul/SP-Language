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
}
