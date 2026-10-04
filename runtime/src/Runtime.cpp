#include "sp/runtime/Runtime.h"
#include <iostream>

extern "C" {

void sp_runtime_init() {
    // Runtime initialization: GC roots, thread pools, allocator init
}

void sp_runtime_shutdown() {
    // Runtime teardown: flush streams, cleanup allocations
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

}
