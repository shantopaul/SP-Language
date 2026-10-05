#include "sp/runtime/Runtime.h"
#include <iostream>
#include <cstdlib>

extern "C" {

void sp_runtime_init() {
    // Runtime initialization: GC roots, thread pools, allocator init
}

void sp_runtime_shutdown() {
    // Runtime teardown: flush streams, cleanup allocations
    std::cout.flush();
    std::cerr.flush();
}

void sp_rt_println_string(const char* str) {
    if (str) {
        std::cout << str << "\n";
    } else {
        std::cout << "null\n";
    }
}

void sp_rt_println_int(int64_t val) {
    std::cout << val << "\n";
}

void sp_rt_println_double(double val) {
    std::cout << val << "\n";
}

void sp_rt_println_bool(bool val) {
    std::cout << (val ? "true" : "false") << "\n";
}

void sp_rt_print_string(const char* str) {
    if (str) {
        std::cout << str;
    } else {
        std::cout << "null";
    }
}

void sp_rt_print_int(int64_t val) {
    std::cout << val;
}

void sp_rt_assert(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "Assertion failed: " << (msg ? msg : "condition was false") << "\n";
        std::abort();
    }
}

void sp_rt_panic(const char* msg) {
    std::cerr << "SP Panic: " << (msg ? msg : "explicit panic") << "\n";
    std::abort();
}

void* sp_rt_alloc(uint64_t size) {
    if (size == 0) size = 1;
    void* ptr = std::calloc(1, static_cast<size_t>(size));
    if (!ptr) {
        std::cerr << "Out of memory in sp_rt_alloc\n";
        std::abort();
    }
    return ptr;
}

void sp_rt_free(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
}

}
