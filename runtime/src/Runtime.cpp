#include "sp/runtime/Runtime.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <atomic>
#include <cassert>
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

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

void sp_rt_println_uint(uint64_t val) {
    std::cout << val << "\n";
}

void sp_rt_println_float(float val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.7g", val);
    std::cout << buf << "\n";
}

void sp_rt_println_double(double val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.16g", val);
    std::cout << buf << "\n";
}

void sp_rt_println_bool(bool val) {
    std::cout << (val ? "true" : "false") << "\n";
}

void sp_rt_println_char(char val) {
    std::cout << val << "\n";
}

void sp_rt_print_string(const char* str) {
    if (str) {
        std::cout << str;
    } else {
        std::cout << "null";
    }
    std::cout.flush();
}

void sp_rt_print_int(int64_t val) {
    std::cout << val;
    std::cout.flush();
}

void sp_rt_print_uint(uint64_t val) {
    std::cout << val;
    std::cout.flush();
}

void sp_rt_print_float(float val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.7g", val);
    std::cout << buf;
    std::cout.flush();
}

void sp_rt_print_double(double val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.16g", val);
    std::cout << buf;
    std::cout.flush();
}

void sp_rt_print_bool(bool val) {
    std::cout << (val ? "true" : "false");
    std::cout.flush();
}

void sp_rt_print_char(char val) {
    std::cout << val;
    std::cout.flush();
}

const char* sp_rt_read_line() {
    std::cout.flush();
    std::string line;
    if (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        char* res = static_cast<char*>(malloc(line.size() + 1));
        if (res) {
            std::memcpy(res, line.c_str(), line.size() + 1);
        }
        return res;
    }
    char* res = static_cast<char*>(malloc(1));
    if (res) res[0] = '\0';
    return res;
}

const char* sp_rt_readln() {
    return sp_rt_read_line();
}

void sp_rt_eprint_string(const char* str) {
    if (str) {
        std::cerr << str;
    } else {
        std::cerr << "null";
    }
    std::cerr.flush();
}

void sp_rt_eprintln_string(const char* str) {
    if (str) {
        std::cerr << str << "\n";
    } else {
        std::cerr << "null\n";
    }
    std::cerr.flush();
}

void sp_rt_flush() {
    std::cout.flush();
}

void sp_rt_eflush() {
    std::cerr.flush();
}

int64_t sp_rt_str_to_int(const char* s) {
    if (!s) return 0;
    try {
        return std::stoll(s, nullptr, 0);
    } catch (...) {
        return 0;
    }
}

double sp_rt_str_to_double(const char* s) {
    if (!s) return 0.0;
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;
    }
}

float sp_rt_str_to_float(const char* s) {
    if (!s) return 0.0f;
    try {
        return std::stof(s);
    } catch (...) {
        return 0.0f;
    }
}

bool sp_rt_str_to_bool(const char* s) {
    if (!s) return false;
    return (std::strcmp(s, "true") == 0 || std::strcmp(s, "1") == 0);
}

const char* sp_rt_str_concat(const char* s1, const char* s2) {
    if (!s1) s1 = "";
    if (!s2) s2 = "";
    size_t l1 = std::strlen(s1);
    size_t l2 = std::strlen(s2);
    char* res = static_cast<char*>(malloc(l1 + l2 + 1));
    if (res) {
        std::memcpy(res, s1, l1);
        std::memcpy(res + l1, s2, l2 + 1);
    }
    return res;
}

bool sp_rt_str_equals(const char* s1, const char* s2) {
    if (s1 == s2) return true;
    if (!s1 || !s2) return false;
    return std::strcmp(s1, s2) == 0;
}

const char* sp_rt_str_from_int(int64_t val) {
    std::string s = std::to_string(val);
    char* res = static_cast<char*>(malloc(s.size() + 1));
    if (res) {
        std::memcpy(res, s.c_str(), s.size() + 1);
    }
    return res;
}

const char* sp_rt_str_from_uint(uint64_t val) {
    std::string s = std::to_string(val);
    char* res = static_cast<char*>(malloc(s.size() + 1));
    if (res) {
        std::memcpy(res, s.c_str(), s.size() + 1);
    }
    return res;
}

const char* sp_rt_str_from_float(float val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.7g", val);
    size_t len = std::strlen(buf);
    char* res = static_cast<char*>(malloc(len + 1));
    if (res) {
        std::memcpy(res, buf, len + 1);
    }
    return res;
}

const char* sp_rt_str_from_double(double val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.16g", val);
    size_t len = std::strlen(buf);
    char* res = static_cast<char*>(malloc(len + 1));
    if (res) {
        std::memcpy(res, buf, len + 1);
    }
    return res;
}

const char* sp_rt_str_from_bool(bool val) {
    const char* s = val ? "true" : "false";
    size_t len = val ? 4 : 5;
    char* res = static_cast<char*>(malloc(len + 1));
    if (res) {
        std::memcpy(res, s, len + 1);
    }
    return res;
}

const char* sp_rt_str_from_char(char val) {
    char* res = static_cast<char*>(malloc(2));
    if (res) {
        res[0] = val;
        res[1] = '\0';
    }
    return res;
}

int64_t sp_rt_str_len(const char* s) {
    if (!s) return 0;
    return static_cast<int64_t>(std::strlen(s));
}

const char* sp_rt_str_upper(const char* s) {
    if (!s) return "";
    size_t len = std::strlen(s);
    char* res = static_cast<char*>(malloc(len + 1));
    if (res) {
        for (size_t i = 0; i < len; ++i) {
            res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
        }
        res[len] = '\0';
    }
    return res;
}

const char* sp_rt_str_lower(const char* s) {
    if (!s) return "";
    size_t len = std::strlen(s);
    char* res = static_cast<char*>(malloc(len + 1));
    if (res) {
        for (size_t i = 0; i < len; ++i) {
            res[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
        }
        res[len] = '\0';
    }
    return res;
}

const char* sp_rt_str_trim(const char* s) {
    if (!s) return "";
    size_t len = std::strlen(s);
    size_t start = 0;
    while (start < len && std::isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }
    size_t end = len;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }
    size_t subLen = end - start;
    char* res = static_cast<char*>(malloc(subLen + 1));
    if (res) {
        std::memcpy(res, s + start, subLen);
        res[subLen] = '\0';
    }
    return res;
}

bool sp_rt_str_contains(const char* s, const char* sub) {
    if (!s || !sub) return false;
    return std::strstr(s, sub) != nullptr;
}

bool sp_rt_str_starts_with(const char* s, const char* prefix) {
    if (!s || !prefix) return false;
    size_t l1 = std::strlen(s);
    size_t l2 = std::strlen(prefix);
    if (l2 > l1) return false;
    return std::strncmp(s, prefix, l2) == 0;
}

bool sp_rt_str_ends_with(const char* s, const char* suffix) {
    if (!s || !suffix) return false;
    size_t l1 = std::strlen(s);
    size_t l2 = std::strlen(suffix);
    if (l2 > l1) return false;
    return std::strcmp(s + (l1 - l2), suffix) == 0;
}

int64_t sp_rt_str_index_of(const char* s, const char* sub) {
    if (!s || !sub) return -1;
    const char* p = std::strstr(s, sub);
    if (!p) return -1;
    return static_cast<int64_t>(p - s);
}

const char* sp_rt_str_substring(const char* s, int64_t start, int64_t end) {
    if (!s) return "";
    size_t len = std::strlen(s);
    if (start < 0) start = 0;
    if (end < start) end = start;
    if (static_cast<size_t>(start) > len) start = len;
    if (static_cast<size_t>(end) > len) end = len;
    size_t subLen = end - start;
    char* res = static_cast<char*>(malloc(subLen + 1));
    if (res) {
        std::memcpy(res, s + start, subLen);
        res[subLen] = '\0';
    }
    return res;
}

const char* sp_rt_str_replace(const char* s, const char* from, const char* to) {
    if (!s) return "";
    if (!from || std::strlen(from) == 0) return s;
    if (!to) to = "";
    std::string str(s);
    std::string fromStr(from);
    std::string toStr(to);
    size_t pos = 0;
    while ((pos = str.find(fromStr, pos)) != std::string::npos) {
        str.replace(pos, fromStr.length(), toStr);
        pos += toStr.length();
    }
    char* res = static_cast<char*>(malloc(str.size() + 1));
    if (res) {
        std::memcpy(res, str.c_str(), str.size() + 1);
    }
    return res;
}

char sp_rt_str_char_at(const char* s, int64_t idx) {
    if (!s || idx < 0 || static_cast<size_t>(idx) >= std::strlen(s)) return '\0';
    return s[idx];
}

void* sp_rt_str_split(const char* s, const char* delim) {
    if (!s) {
        return sp_rt_array_alloc(0, sizeof(int64_t));
    }
    std::string str(s);
    std::string d(delim ? delim : "");
    std::vector<std::string> parts;
    if (d.empty()) {
        for (char c : str) {
            parts.push_back(std::string(1, c));
        }
    } else {
        size_t start = 0;
        size_t pos = str.find(d);
        while (pos != std::string::npos) {
            parts.push_back(str.substr(start, pos - start));
            start = pos + d.length();
            pos = str.find(d, start);
        }
        parts.push_back(str.substr(start));
    }
    void* arr = sp_rt_array_alloc(parts.size(), sizeof(int64_t));
    for (size_t i = 0; i < parts.size(); ++i) {
        char* copy = static_cast<char*>(malloc(parts[i].size() + 1));
        std::memcpy(copy, parts[i].c_str(), parts[i].size() + 1);
        sp_rt_array_set_i64(arr, i, reinterpret_cast<int64_t>(copy));
    }
    return arr;
}

const char* sp_rt_str_reverse(const char* s) {
    if (!s) return "";
    std::string str(s);
    std::reverse(str.begin(), str.end());
    char* copy = static_cast<char*>(malloc(str.size() + 1));
    std::memcpy(copy, str.c_str(), str.size() + 1);
    return copy;
}

const char* sp_rt_str_repeat(const char* s, int64_t count) {
    if (!s || count <= 0) return "";
    std::string str(s);
    std::string res;
    res.reserve(str.size() * count);
    for (int64_t i = 0; i < count; ++i) {
        res += str;
    }
    char* copy = static_cast<char*>(malloc(res.size() + 1));
    std::memcpy(copy, res.c_str(), res.size() + 1);
    return copy;
}

bool sp_rt_str_is_empty(const char* s) {
    return !s || *s == '\0';
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

static std::atomic<uint64_t> g_allocated_bytes{0};
static std::atomic<uint64_t> g_active_arc_objects{0};

void* sp_rt_alloc(uint64_t size) {
    if (size == 0) size = 1;
    void* ptr = std::calloc(1, static_cast<size_t>(size));
    if (!ptr) {
        std::cerr << "Out of memory in sp_rt_alloc\n";
        std::abort();
    }
    g_allocated_bytes.fetch_add(size, std::memory_order_relaxed);
    return ptr;
}

void sp_rt_free(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
}

// ---------------------------------------------------------------------------
// Phase 11: Deterministic ARC and Object Lifecycle Management
// ---------------------------------------------------------------------------

constexpr uint32_t SP_ARC_MAGIC = 0x53504152; // 'SPAR'

struct SPObjectHeader {
    uint32_t magic;
    std::atomic<uint32_t> strong_count;
    std::atomic<uint32_t> weak_count;
    uint32_t allocated_size;
    SPDestructorFn destructor;
};

void* sp_rt_arc_alloc(uint64_t size, SPDestructorFn dtor) {
    if (size == 0) size = 1;
    size_t total_size = sizeof(SPObjectHeader) + static_cast<size_t>(size);
    auto* header = static_cast<SPObjectHeader*>(std::malloc(total_size));
    if (!header) {
        std::cerr << "Out of memory in sp_rt_arc_alloc\n";
        std::abort();
    }

    header->magic = SP_ARC_MAGIC;
    header->strong_count.store(1, std::memory_order_relaxed);
    header->weak_count.store(1, std::memory_order_relaxed); // Strong refs hold 1 weak ref
    header->allocated_size = static_cast<uint32_t>(size);
    header->destructor = dtor;

    g_allocated_bytes.fetch_add(total_size, std::memory_order_relaxed);
    g_active_arc_objects.fetch_add(1, std::memory_order_relaxed);

    void* payload = reinterpret_cast<uint8_t*>(header) + sizeof(SPObjectHeader);
    std::memset(payload, 0, static_cast<size_t>(size));
    return payload;
}

static inline SPObjectHeader* get_arc_header(void* ptr) {
    if (!ptr) return nullptr;
    auto* header = reinterpret_cast<SPObjectHeader*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(SPObjectHeader));
    return (header->magic == SP_ARC_MAGIC) ? header : nullptr;
}

void* sp_rt_retain(void* ptr) {
    auto* header = get_arc_header(ptr);
    if (header) {
        header->strong_count.fetch_add(1, std::memory_order_relaxed);
    }
    return ptr;
}

void sp_rt_release(void* ptr) {
    auto* header = get_arc_header(ptr);
    if (!header) return;

    if (header->strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // Last strong reference released: invoke destructor
        if (header->destructor) {
            header->destructor(ptr);
        }
        g_active_arc_objects.fetch_sub(1, std::memory_order_relaxed);

        // Release the weak reference held on behalf of all strong references
        sp_rt_weak_release(header);
    }
}

uint32_t sp_rt_strong_count(void* ptr) {
    auto* header = get_arc_header(ptr);
    return header ? header->strong_count.load(std::memory_order_relaxed) : 0;
}

uint32_t sp_rt_weak_count(void* ptr) {
    auto* header = get_arc_header(ptr);
    return header ? header->weak_count.load(std::memory_order_relaxed) : 0;
}

// ---------------------------------------------------------------------------
// Phase 11: Weak References (Cycle-breaking)
// ---------------------------------------------------------------------------

void* sp_rt_weak_create(void* strong_ptr) {
    auto* header = get_arc_header(strong_ptr);
    if (!header) return nullptr;

    header->weak_count.fetch_add(1, std::memory_order_relaxed);
    return header;
}

void* sp_rt_weak_retain(void* weak_ptr) {
    if (!weak_ptr) return nullptr;
    auto* header = static_cast<SPObjectHeader*>(weak_ptr);
    if (header->magic == SP_ARC_MAGIC) {
        header->weak_count.fetch_add(1, std::memory_order_relaxed);
    }
    return weak_ptr;
}

void sp_rt_weak_release(void* weak_ptr) {
    if (!weak_ptr) return;
    auto* header = static_cast<SPObjectHeader*>(weak_ptr);
    if (header->magic != SP_ARC_MAGIC) return;

    if (header->weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // Last weak reference dropped: free the memory block
        size_t total_size = sizeof(SPObjectHeader) + header->allocated_size;
        g_allocated_bytes.fetch_sub(total_size, std::memory_order_relaxed);
        header->magic = 0; // invalidate
        std::free(header);
    }
}

void* sp_rt_weak_upgrade(void* weak_ptr) {
    if (!weak_ptr) return nullptr;
    auto* header = static_cast<SPObjectHeader*>(weak_ptr);
    if (header->magic != SP_ARC_MAGIC) return nullptr;

    uint32_t count = header->strong_count.load(std::memory_order_relaxed);
    while (count != 0) {
        if (header->strong_count.compare_exchange_weak(count, count + 1,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed)) {
            return reinterpret_cast<uint8_t*>(header) + sizeof(SPObjectHeader);
        }
    }
    return nullptr; // Strong object has already been destroyed
}

// ---------------------------------------------------------------------------
// Phase 11: Memory Arena Subsystem (std.mem.Arena)
// ---------------------------------------------------------------------------

struct SPArenaBlock {
    uint8_t* buffer;
    uint64_t capacity;
    uint64_t offset;
    SPArenaBlock* next;
};

struct SPArena {
    SPArenaBlock* head;
    SPArenaBlock* current;
    uint64_t default_block_size;
    uint64_t total_allocated;
};

void* sp_rt_arena_create(uint64_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 64 * 1024; // 64 KB default

    auto* arena = new SPArena();
    arena->default_block_size = initial_capacity;
    arena->total_allocated = 0;

    auto* block = new SPArenaBlock();
    block->capacity = initial_capacity;
    block->offset = 0;
    block->next = nullptr;
    block->buffer = static_cast<uint8_t*>(std::malloc(initial_capacity));

    arena->head = block;
    arena->current = block;

    g_allocated_bytes.fetch_add(sizeof(SPArena) + sizeof(SPArenaBlock) + initial_capacity, std::memory_order_relaxed);
    return arena;
}

void* sp_rt_arena_alloc(void* arena_ptr, uint64_t size, uint64_t alignment) {
    if (!arena_ptr || size == 0) return nullptr;
    if (alignment == 0) alignment = 8;

    auto* arena = static_cast<SPArena*>(arena_ptr);
    auto* cur = arena->current;

    // Calculate aligned offset
    uint64_t aligned_offset = (cur->offset + (alignment - 1)) & ~(alignment - 1);
    if (aligned_offset + size <= cur->capacity) {
        cur->offset = aligned_offset + size;
        arena->total_allocated += size;
        return cur->buffer + aligned_offset;
    }

    // Need new block
    uint64_t new_cap = std::max(arena->default_block_size, size + alignment);
    auto* new_block = new SPArenaBlock();
    new_block->capacity = new_cap;
    new_block->offset = size;
    new_block->next = nullptr;
    new_block->buffer = static_cast<uint8_t*>(std::malloc(new_cap));

    cur->next = new_block;
    arena->current = new_block;
    arena->total_allocated += size;

    g_allocated_bytes.fetch_add(sizeof(SPArenaBlock) + new_cap, std::memory_order_relaxed);
    return new_block->buffer;
}

void sp_rt_arena_reset(void* arena_ptr) {
    if (!arena_ptr) return;
    auto* arena = static_cast<SPArena*>(arena_ptr);
    auto* blk = arena->head;
    while (blk) {
        blk->offset = 0;
        blk = blk->next;
    }
    arena->current = arena->head;
    arena->total_allocated = 0;
}

void sp_rt_arena_destroy(void* arena_ptr) {
    if (!arena_ptr) return;
    auto* arena = static_cast<SPArena*>(arena_ptr);
    auto* blk = arena->head;
    while (blk) {
        auto* next = blk->next;
        g_allocated_bytes.fetch_sub(sizeof(SPArenaBlock) + blk->capacity, std::memory_order_relaxed);
        std::free(blk->buffer);
        delete blk;
        blk = next;
    }
    g_allocated_bytes.fetch_sub(sizeof(SPArena), std::memory_order_relaxed);
    delete arena;
}

uint64_t sp_rt_arena_allocated(void* arena_ptr) {
    if (!arena_ptr) return 0;
    return static_cast<SPArena*>(arena_ptr)->total_allocated;
}

uint64_t sp_rt_arena_allocated_bytes(void* arena_ptr) {
    return sp_rt_arena_allocated(arena_ptr);
}

uint64_t sp_rt_mem_allocated_bytes() {
    return g_allocated_bytes.load(std::memory_order_relaxed);
}

uint64_t sp_rt_mem_active_arc_objects() {
    return g_active_arc_objects.load(std::memory_order_relaxed);
}

uint64_t sp_rt_mem_live_objects() {
    return sp_rt_mem_active_arc_objects();
}

// Internal structures
struct SPArrayHeader {
    int64_t length;
    int64_t capacity;
};

struct SPListImpl {
    int64_t element_size;
    std::vector<int64_t> items;
};

struct SPMapImpl {
    std::unordered_map<std::string, int64_t> str_map;
    std::unordered_map<int64_t, int64_t> int_map;
};

struct SPSetImpl {
    std::unordered_set<std::string> str_set;
    std::unordered_set<int64_t> int_set;
};

// Native Array primitives
void* sp_rt_array_alloc(int64_t length, int64_t element_size) {
    if (length < 0) length = 0;
    if (element_size <= 0) element_size = 8;
    size_t total_size = sizeof(SPArrayHeader) + static_cast<size_t>(length * element_size);
    auto* header = static_cast<SPArrayHeader*>(sp_rt_alloc(total_size));
    header->length = length;
    header->capacity = length;
    return header;
}

int64_t sp_rt_array_length(void* arr) {
    if (!arr) return 0;
    return static_cast<SPArrayHeader*>(arr)->length;
}

void* sp_rt_array_data(void* arr) {
    if (!arr) return nullptr;
    return reinterpret_cast<uint8_t*>(arr) + sizeof(SPArrayHeader);
}

void* sp_rt_array_get_ptr(void* arr, int64_t index, int64_t element_size) {
    if (!arr) {
        sp_rt_panic("NullPointerException: array is null");
    }
    auto* header = static_cast<SPArrayHeader*>(arr);
    if (index < 0 || index >= header->length) {
        std::string msg = "ArrayIndexOutOfBoundsException: index " + std::to_string(index) +
                          " out of bounds for length " + std::to_string(header->length);
        sp_rt_panic(msg.c_str());
    }
    return reinterpret_cast<uint8_t*>(arr) + sizeof(SPArrayHeader) + (index * element_size);
}

int64_t sp_rt_array_get_i64(void* arr, int64_t index) {
    void* ptr = sp_rt_array_get_ptr(arr, index, sizeof(int64_t));
    return *reinterpret_cast<int64_t*>(ptr);
}

void sp_rt_array_set_i64(void* arr, int64_t index, int64_t val) {
    void* ptr = sp_rt_array_get_ptr(arr, index, sizeof(int64_t));
    *reinterpret_cast<int64_t*>(ptr) = val;
}

double sp_rt_array_get_f64(void* arr, int64_t index) {
    void* ptr = sp_rt_array_get_ptr(arr, index, sizeof(double));
    return *reinterpret_cast<double*>(ptr);
}

void sp_rt_array_set_f64(void* arr, int64_t index, double val) {
    void* ptr = sp_rt_array_get_ptr(arr, index, sizeof(double));
    *reinterpret_cast<double*>(ptr) = val;
}

// Native List<T> primitives
void* sp_rt_list_alloc(int64_t element_size, int64_t initial_capacity) {
    (void)initial_capacity;
    auto* list = new SPListImpl();
    list->element_size = element_size > 0 ? element_size : 8;
    return list;
}

int64_t sp_rt_list_size(void* list) {
    if (!list) return 0;
    return static_cast<int64_t>(static_cast<SPListImpl*>(list)->items.size());
}

void sp_rt_list_push_i64(void* list, int64_t val) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    static_cast<SPListImpl*>(list)->items.push_back(val);
}

int64_t sp_rt_list_get_i64(void* list, int64_t index) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    auto* impl = static_cast<SPListImpl*>(list);
    if (index < 0 || static_cast<size_t>(index) >= impl->items.size()) {
        sp_rt_panic("IndexOutOfBoundsException: index out of bounds in list.get");
    }
    return impl->items[index];
}

void sp_rt_list_set_i64(void* list, int64_t index, int64_t val) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    auto* impl = static_cast<SPListImpl*>(list);
    if (index < 0 || static_cast<size_t>(index) >= impl->items.size()) {
        sp_rt_panic("IndexOutOfBoundsException: index out of bounds in list.set");
    }
    impl->items[index] = val;
}

int64_t sp_rt_list_pop_i64(void* list) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    auto* impl = static_cast<SPListImpl*>(list);
    if (impl->items.empty()) {
        sp_rt_panic("NoSuchElementException: cannot pop from empty list");
    }
    int64_t val = impl->items.back();
    impl->items.pop_back();
    return val;
}

void sp_rt_list_clear(void* list) {
    if (!list) return;
    static_cast<SPListImpl*>(list)->items.clear();
}

bool sp_rt_list_contains_i64(void* list, int64_t val) {
    if (!list) return false;
    auto& items = static_cast<SPListImpl*>(list)->items;
    return std::find(items.begin(), items.end(), val) != items.end();
}

void sp_rt_list_remove_at(void* list, int64_t index) {
    if (!list) return;
    auto* impl = static_cast<SPListImpl*>(list);
    if (index >= 0 && static_cast<size_t>(index) < impl->items.size()) {
        impl->items.erase(impl->items.begin() + index);
    }
}

bool sp_rt_list_is_empty(void* list) {
    if (!list) return true;
    return static_cast<SPListImpl*>(list)->items.empty();
}

// Native Map<K, V> primitives
void* sp_rt_map_alloc() {
    return new SPMapImpl();
}

int64_t sp_rt_map_size(void* map) {
    if (!map) return 0;
    auto* impl = static_cast<SPMapImpl*>(map);
    return static_cast<int64_t>(impl->str_map.size() + impl->int_map.size());
}

void sp_rt_map_set_str(void* map, const char* key, int64_t val) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    if (!key) return;
    static_cast<SPMapImpl*>(map)->str_map[key] = val;
}

int64_t sp_rt_map_get_str(void* map, const char* key) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    if (!key) return 0;
    auto& m = static_cast<SPMapImpl*>(map)->str_map;
    auto it = m.find(key);
    return it != m.end() ? it->second : 0;
}

bool sp_rt_map_contains_str(void* map, const char* key) {
    if (!map || !key) return false;
    auto& m = static_cast<SPMapImpl*>(map)->str_map;
    return m.find(key) != m.end();
}

void sp_rt_map_set_i64(void* map, int64_t key, int64_t val) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    static_cast<SPMapImpl*>(map)->int_map[key] = val;
}

int64_t sp_rt_map_get_i64(void* map, int64_t key) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    auto& m = static_cast<SPMapImpl*>(map)->int_map;
    auto it = m.find(key);
    return it != m.end() ? it->second : 0;
}

bool sp_rt_map_contains_i64(void* map, int64_t key) {
    if (!map) return false;
    auto& m = static_cast<SPMapImpl*>(map)->int_map;
    return m.find(key) != m.end();
}

void sp_rt_map_remove_str(void* map, const char* key) {
    if (!map || !key) return;
    static_cast<SPMapImpl*>(map)->str_map.erase(key);
}

void sp_rt_map_remove_i64(void* map, int64_t key) {
    if (!map) return;
    static_cast<SPMapImpl*>(map)->int_map.erase(key);
}

void sp_rt_map_clear(void* map) {
    if (!map) return;
    auto* impl = static_cast<SPMapImpl*>(map);
    impl->str_map.clear();
    impl->int_map.clear();
}

bool sp_rt_map_is_empty(void* map) {
    if (!map) return true;
    auto* impl = static_cast<SPMapImpl*>(map);
    return impl->str_map.empty() && impl->int_map.empty();
}

// Native Set<T> primitives
void* sp_rt_set_alloc() {
    return new SPSetImpl();
}

int64_t sp_rt_set_size(void* set) {
    if (!set) return 0;
    auto* impl = static_cast<SPSetImpl*>(set);
    return static_cast<int64_t>(impl->str_set.size() + impl->int_set.size());
}

void sp_rt_set_add_i64(void* set, int64_t val) {
    if (!set) sp_rt_panic("NullPointerException: set is null");
    static_cast<SPSetImpl*>(set)->int_set.insert(val);
}

void sp_rt_set_add_str(void* set, const char* val) {
    if (!set) sp_rt_panic("NullPointerException: set is null");
    if (!val) return;
    static_cast<SPSetImpl*>(set)->str_set.insert(val);
}

bool sp_rt_set_contains_i64(void* set, int64_t val) {
    if (!set) return false;
    auto& s = static_cast<SPSetImpl*>(set)->int_set;
    return s.find(val) != s.end();
}

bool sp_rt_set_contains_str(void* set, const char* val) {
    if (!set || !val) return false;
    auto& s = static_cast<SPSetImpl*>(set)->str_set;
    return s.find(val) != s.end();
}

void sp_rt_set_remove_i64(void* set, int64_t val) {
    if (!set) return;
    static_cast<SPSetImpl*>(set)->int_set.erase(val);
}

void sp_rt_set_remove_str(void* set, const char* val) {
    if (!set || !val) return;
    static_cast<SPSetImpl*>(set)->str_set.erase(val);
}

void sp_rt_set_clear(void* set) {
    if (!set) return;
    auto* impl = static_cast<SPSetImpl*>(set);
    impl->str_set.clear();
    impl->int_set.clear();
}

bool sp_rt_set_is_empty(void* set) {
    if (!set) return true;
    auto* impl = static_cast<SPSetImpl*>(set);
    return impl->str_set.empty() && impl->int_set.empty();
}

bool sp_rt_array_is_empty(void* arr) {
    if (!arr) return true;
    return sp_rt_array_length(arr) == 0;
}

// Higher-order collection functions
void* sp_rt_array_map(void* arr, int64_t (*fn)(int64_t)) {
    if (!arr) return nullptr;
    int64_t len = sp_rt_array_length(arr);
    void* result = sp_rt_array_alloc(len, sizeof(int64_t));
    if (!fn) return result;
    for (int64_t i = 0; i < len; ++i) {
        int64_t val = sp_rt_array_get_i64(arr, i);
        int64_t transformed = fn(val);
        sp_rt_array_set_i64(result, i, transformed);
    }
    return result;
}

void* sp_rt_array_filter(void* arr, int64_t (*predicate)(int64_t)) {
    if (!arr) return nullptr;
    int64_t len = sp_rt_array_length(arr);
    int64_t match_count = 0;
    for (int64_t i = 0; i < len; ++i) {
        int64_t val = sp_rt_array_get_i64(arr, i);
        if (predicate && predicate(val)) {
            match_count++;
        }
    }
    void* result = sp_rt_array_alloc(match_count, sizeof(int64_t));
    int64_t out_idx = 0;
    for (int64_t i = 0; i < len; ++i) {
        int64_t val = sp_rt_array_get_i64(arr, i);
        if (predicate && predicate(val)) {
            sp_rt_array_set_i64(result, out_idx++, val);
        }
    }
    return result;
}

int64_t sp_rt_array_reduce(void* arr, int64_t (*fn)(int64_t, int64_t), int64_t initial) {
    if (!arr) return initial;
    int64_t len = sp_rt_array_length(arr);
    int64_t acc = initial;
    for (int64_t i = 0; i < len; ++i) {
        int64_t val = sp_rt_array_get_i64(arr, i);
        acc = fn ? fn(acc, val) : acc + val;
    }
    return acc;
}

void* sp_rt_list_map(void* list, int64_t (*fn)(int64_t)) {
    if (!list) return nullptr;
    int64_t sz = sp_rt_list_size(list);
    void* result = sp_rt_list_alloc(sizeof(int64_t), sz);
    if (!fn) return result;
    for (int64_t i = 0; i < sz; ++i) {
        int64_t val = sp_rt_list_get_i64(list, i);
        sp_rt_list_push_i64(result, fn(val));
    }
    return result;
}

void* sp_rt_list_filter(void* list, int64_t (*predicate)(int64_t)) {
    if (!list) return nullptr;
    int64_t sz = sp_rt_list_size(list);
    void* result = sp_rt_list_alloc(sizeof(int64_t), sz);
    for (int64_t i = 0; i < sz; ++i) {
        int64_t val = sp_rt_list_get_i64(list, i);
        if (predicate && predicate(val)) {
            sp_rt_list_push_i64(result, val);
        }
    }
    return result;
}

int64_t sp_rt_list_reduce(void* list, int64_t (*fn)(int64_t, int64_t), int64_t initial) {
    if (!list) return initial;
    int64_t sz = sp_rt_list_size(list);
    int64_t acc = initial;
    for (int64_t i = 0; i < sz; ++i) {
        int64_t val = sp_rt_list_get_i64(list, i);
        acc = fn ? fn(acc, val) : acc + val;
    }
    return acc;
}

double sp_rt_math_sqrt(double x) {
    return std::sqrt(x);
}

double sp_rt_math_pow(double base, double exp) {
    return std::pow(base, exp);
}

double sp_rt_math_sin(double x) {
    return std::sin(x);
}

double sp_rt_math_cos(double x) {
    return std::cos(x);
}

double sp_rt_math_tan(double x) {
    return std::tan(x);
}

double sp_rt_math_asin(double x) {
    return std::asin(x);
}

double sp_rt_math_acos(double x) {
    return std::acos(x);
}

double sp_rt_math_atan(double x) {
    return std::atan(x);
}

double sp_rt_math_atan2(double y, double x) {
    return std::atan2(y, x);
}

double sp_rt_math_floor(double x) {
    return std::floor(x);
}

double sp_rt_math_ceil(double x) {
    return std::ceil(x);
}

double sp_rt_math_round(double x) {
    return std::round(x);
}

double sp_rt_math_abs_double(double x) {
    return std::fabs(x);
}

int64_t sp_rt_math_abs_int(int64_t x) {
    return std::abs(x);
}

double sp_rt_math_min_double(double a, double b) {
    return std::fmin(a, b);
}

int64_t sp_rt_math_min_int(int64_t a, int64_t b) {
    return a < b ? a : b;
}

double sp_rt_math_max_double(double a, double b) {
    return std::fmax(a, b);
}

int64_t sp_rt_math_max_int(int64_t a, int64_t b) {
    return a > b ? a : b;
}

double sp_rt_math_log(double x) {
    return std::log(x);
}

double sp_rt_math_log10(double x) {
    return std::log10(x);
}

double sp_rt_math_log2(double x) {
    return std::log2(x);
}

double sp_rt_math_exp(double x) {
    return std::exp(x);
}

// File system functions (std.fs)
const char* sp_rt_fs_read_text(const char* path) {
    if (!path) return "";
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    char* copy = static_cast<char*>(malloc(content.size() + 1));
    std::memcpy(copy, content.c_str(), content.size() + 1);
    return copy;
}

bool sp_rt_fs_write_text(const char* path, const char* content) {
    if (!path) return false;
    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    if (content) {
        file.write(content, std::strlen(content));
    }
    return file.good();
}

bool sp_rt_fs_append_text(const char* path, const char* content) {
    if (!path) return false;
    std::ofstream file(path, std::ios::out | std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    if (content) {
        file.write(content, std::strlen(content));
    }
    return file.good();
}

bool sp_rt_fs_exists(const char* path) {
    if (!path) return false;
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool sp_rt_fs_delete(const char* path) {
    if (!path) return false;
    std::error_code ec;
    return std::filesystem::remove_all(path, ec) > 0;
}

bool sp_rt_fs_create_dir(const char* path) {
    if (!path) return false;
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return !ec;
}

void* sp_rt_fs_list_dir(const char* path) {
    if (!path) {
        return sp_rt_array_alloc(0, sizeof(int64_t));
    }
    std::vector<std::string> entries;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        entries.push_back(entry.path().filename().string());
    }
    if (ec) {
        return sp_rt_array_alloc(0, sizeof(int64_t));
    }
    std::sort(entries.begin(), entries.end());
    void* arr = sp_rt_array_alloc(entries.size(), sizeof(int64_t));
    for (size_t i = 0; i < entries.size(); ++i) {
        char* copy = static_cast<char*>(malloc(entries[i].size() + 1));
        std::memcpy(copy, entries[i].c_str(), entries[i].size() + 1);
        sp_rt_array_set_i64(arr, i, reinterpret_cast<int64_t>(copy));
    }
    return arr;
}

bool sp_rt_fs_is_file(const char* path) {
    if (!path) return false;
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool sp_rt_fs_is_dir(const char* path) {
    if (!path) return false;
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

// Time functions (std.time)
int64_t sp_rt_time_now_ms() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

int64_t sp_rt_time_now_secs() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

int64_t sp_rt_time_now_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

void sp_rt_time_sleep_ms(int64_t ms) {
    if (ms > 0) {
#if defined(_WIN32) || defined(_WIN64)
        ::Sleep(static_cast<DWORD>(ms));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
    }
}

void sp_rt_time_sleep_secs(int64_t s) {
    if (s > 0) {
#if defined(_WIN32) || defined(_WIN64)
        ::Sleep(static_cast<DWORD>(s * 1000));
#else
        std::this_thread::sleep_for(std::chrono::seconds(s));
#endif
    }
}

const char* sp_rt_time_format_iso(int64_t timestamp) {
    std::time_t t = static_cast<std::time_t>(timestamp > 100000000000LL ? timestamp / 1000 : timestamp);
    std::tm tm_buf{};
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    char* copy = static_cast<char*>(malloc(std::strlen(buf) + 1));
    std::strcpy(copy, buf);
    return copy;
}

int64_t sp_rt_time_parse_iso(const char* iso_str) {
    if (!iso_str) return 0;
    std::tm tm_buf{};
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    if (sscanf(iso_str, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec) >= 3) {
        tm_buf.tm_year = year - 1900;
        tm_buf.tm_mon = month - 1;
        tm_buf.tm_mday = day;
        tm_buf.tm_hour = hour;
        tm_buf.tm_min = min;
        tm_buf.tm_sec = sec;
#if defined(_WIN32) || defined(_WIN64)
        std::time_t t = _mkgmtime(&tm_buf);
#else
        std::time_t t = timegm(&tm_buf);
#endif
        if (t != static_cast<std::time_t>(-1)) {
            return static_cast<int64_t>(t);
        }
    }
    return 0;
}

// Duration functions (std.time)
struct SPDurationImpl {
    int64_t millis;
};

void* sp_rt_duration_from_millis(int64_t ms) {
    auto* d = new SPDurationImpl();
    d->millis = ms;
    return d;
}

void* sp_rt_duration_from_seconds(int64_t s) {
    return sp_rt_duration_from_millis(s * 1000);
}

void* sp_rt_duration_from_minutes(int64_t m) {
    return sp_rt_duration_from_millis(m * 60000);
}

void* sp_rt_duration_from_hours(int64_t h) {
    return sp_rt_duration_from_millis(h * 3600000);
}

int64_t sp_rt_duration_to_millis(void* d) {
    if (!d) return 0;
    return static_cast<SPDurationImpl*>(d)->millis;
}

int64_t sp_rt_duration_to_seconds(void* d) {
    if (!d) return 0;
    return static_cast<SPDurationImpl*>(d)->millis / 1000;
}

// OS runtime functions (std.os)
const char* sp_rt_os_get_env(const char* name) {
    if (!name) return "";
    const char* val = std::getenv(name);
    if (!val) return "";
    char* copy = static_cast<char*>(malloc(std::strlen(val) + 1));
    std::memcpy(copy, val, std::strlen(val) + 1);
    return copy;
}

bool sp_rt_os_set_env(const char* name, const char* val) {
    if (!name) return false;
    const char* actual_val = val ? val : "";
#if defined(_WIN32)
    return _putenv_s(name, actual_val) == 0;
#else
    return setenv(name, actual_val, 1) == 0;
#endif
}

const char* sp_rt_os_platform() {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

const char* sp_rt_os_arch() {
#if defined(__x86_64__) || defined(_M_X64)
    return "x64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#else
    return "unknown";
#endif
}

void* sp_rt_os_args() {
#if defined(_WIN32)
    int count = __argc > 0 ? __argc : 0;
    void* arr = sp_rt_array_alloc(static_cast<int64_t>(count), sizeof(int64_t));
    for (int i = 0; i < count; ++i) {
        const char* arg_str = (__argv && __argv[i]) ? __argv[i] : "";
        size_t len = std::strlen(arg_str);
        char* copy = static_cast<char*>(malloc(len + 1));
        std::memcpy(copy, arg_str, len + 1);
        sp_rt_array_set_i64(arr, static_cast<int64_t>(i), reinterpret_cast<int64_t>(copy));
    }
    return arr;
#else
    return sp_rt_array_alloc(0, sizeof(int64_t));
#endif
}

void sp_rt_os_exit(int64_t code) {
    std::exit(static_cast<int>(code));
}

// Process runtime functions (std.process)
int64_t sp_rt_process_exec(const char* cmd) {
    if (!cmd) return -1;
    int ret = std::system(cmd);
#if !defined(_WIN32)
    if (WIFEXITED(ret)) {
        return static_cast<int64_t>(WEXITSTATUS(ret));
    }
#endif
    return static_cast<int64_t>(ret);
}

int64_t sp_rt_process_spawn(const char* cmd) {
    if (!cmd) return -1;
#if defined(_WIN32)
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    std::vector<char> cmdBuf(cmd, cmd + std::strlen(cmd) + 1);
    if (CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        int64_t pid = static_cast<int64_t>(pi.dwProcessId);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return pid;
    }
    return -1;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        _exit(127);
    }
    return static_cast<int64_t>(pid);
#endif
}

const char* sp_rt_process_run(const char* cmd) {
    if (!cmd) return "";
#if defined(_WIN32)
    FILE* pipe = _popen(cmd, "r");
#else
    FILE* pipe = popen(cmd, "r");
#endif
    if (!pipe) return "";
    std::string result;
    char buffer[256];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    char* copy = static_cast<char*>(malloc(result.size() + 1));
    std::memcpy(copy, result.c_str(), result.size() + 1);
    return copy;
}

int64_t sp_rt_process_pid() {
#if defined(_WIN32)
    return static_cast<int64_t>(GetCurrentProcessId());
#else
    return static_cast<int64_t>(getpid());
#endif
}

}

