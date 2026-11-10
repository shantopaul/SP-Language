#include "sp/runtime/Runtime.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <atomic>
#include <mutex>
#include <unordered_set>
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
    // Phase 6 slice B: strict parsing — the old code returned 0 for any
    // invalid input (and silently parsed "0x.." as hex / "0.." as octal
    // via base 0, and ignored trailing junk like "42abc"). A plain `int`
    // cannot express failure, so invalid input is a loud panic naming
    // the input instead of a silent wrong value. Leading/trailing
    // whitespace is trimmed (readLine() already strips the newline).
    if (!s) sp_rt_panic("int.parse: null input");
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
    if (*s == '\0') sp_rt_panic("int.parse: empty input");
    try {
        size_t pos = 0;
        int64_t v = std::stoll(s, &pos, 10);
        while (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n') ++pos;
        if (s[pos] != '\0') {
            std::string msg = "int.parse: invalid integer '";
            msg += s;
            msg += "'";
            sp_rt_panic(msg.c_str());
        }
        return v;
    } catch (const std::out_of_range&) {
        std::string msg = "int.parse: integer out of range '";
        msg += s;
        msg += "'";
        sp_rt_panic(msg.c_str());
    } catch (...) {
        std::string msg = "int.parse: invalid integer '";
        msg += s;
        msg += "'";
        sp_rt_panic(msg.c_str());
    }
}

double sp_rt_str_to_double(const char* s) {
    // Phase 6 slice B: strict like sp_rt_str_to_int (was: silent 0.0).
    if (!s) sp_rt_panic("double.parse: null input");
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
    if (*s == '\0') sp_rt_panic("double.parse: empty input");
    try {
        size_t pos = 0;
        double v = std::stod(s, &pos);
        while (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n') ++pos;
        if (s[pos] != '\0') {
            std::string msg = "double.parse: invalid number '";
            msg += s;
            msg += "'";
            sp_rt_panic(msg.c_str());
        }
        return v;
    } catch (...) {
        std::string msg = "double.parse: invalid number '";
        msg += s;
        msg += "'";
        sp_rt_panic(msg.c_str());
    }
}

float sp_rt_str_to_float(const char* s) {
    // Phase 6 slice B: strict like sp_rt_str_to_int (was: silent 0.0f).
    if (!s) sp_rt_panic("float.parse: null input");
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
    if (*s == '\0') sp_rt_panic("float.parse: empty input");
    try {
        size_t pos = 0;
        float v = std::stof(s, &pos);
        while (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n') ++pos;
        if (s[pos] != '\0') {
            std::string msg = "float.parse: invalid number '";
            msg += s;
            msg += "'";
            sp_rt_panic(msg.c_str());
        }
        return v;
    } catch (...) {
        std::string msg = "float.parse: invalid number '";
        msg += s;
        msg += "'";
        sp_rt_panic(msg.c_str());
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

// Phase 37 slice B: sp_rt_free is ARC-aware — see the definition
// after sp_rt_release (it must live below get_arc_header).

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

// Parent fix (Phase 37 merge): live ARC payload registry.
//
// get_arc_header used to decide "is this an ARC payload?" by reading
// the 4-byte magic 24 bytes BEFORE the pointer. That read is only
// safe when those bytes are mapped — and a plain malloc'd buffer of
// >=128 KiB is an mmap'd region whose base has NO mapped bytes
// before it, so the probe segfaults. Slice B made sp_rt_free (which
// probes) the free path for every container's internal buffer, and
// slice A made scope drops actually run container destructors —
// together they crashed CollectionStressAndScalingNativeExecution:
// a List<int> grown past 8192 elements (items = 128 KiB mmap'd)
// segfaulted in sp_list_destructor -> sp_rt_free -> get_arc_header.
// Small lists never crashed (heap chunk headers are mapped, the
// magic simply mismatched), which is why slice B's own proofs and
// ctest passed: its tree never dropped a large container at all
// (main's locals were never dropped before slice A).
//
// The registry is the discriminator now: only pointers returned by
// sp_rt_arc_alloc are members, and membership is checked BEFORE any
// memory is read, so a plain (even mmap'd) pointer is a safe
// miss. The entry is erased when the header block is actually freed
// (sp_rt_weak_release, last weak ref). Cost: one mutex + hash lookup
// per retain/release/free — the lock is uncontended in
// single-threaded execution and is never held across a destructor
// call, so no nesting or deadlock is possible.
static std::mutex g_arc_registry_mutex;
static std::unordered_set<void*> g_arc_payloads;

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
    // Parent fix (Phase 37 merge): register the live payload — see
    // get_arc_header above.
    {
        std::lock_guard<std::mutex> lock(g_arc_registry_mutex);
        g_arc_payloads.insert(payload);
    }
    std::memset(payload, 0, static_cast<size_t>(size));
    return payload;
}

static inline SPObjectHeader* get_arc_header(void* ptr) {
    if (!ptr) return nullptr;
    {
        std::lock_guard<std::mutex> lock(g_arc_registry_mutex);
        if (g_arc_payloads.find(ptr) == g_arc_payloads.end()) return nullptr;
    }
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

// Phase 37 slice B: sp_rt_free is the runtime's universal free —
// it must accept any pointer the runtime handed out. Containers
// are ARC objects now (sp_rt_arc_alloc), so their payload pointer
// is 16 bytes past the malloc base and std::free on it aborts
// ("free(): invalid pointer"). Route ARC pointers through
// sp_rt_release instead: at count 1 that frees (header +
// payload, destructor first); at count > 1 it only decrements,
// which is the safe direction — never freeing under live shares.
// Plain pointers (strings, buffers, frames) keep the old
// std::free path. C/C++ callers that used to std::free a runtime
// array must use sp_rt_free (see AsyncTest CAbi tests).
void sp_rt_free(void* ptr) {
    if (!ptr) return;
    if (get_arc_header(ptr)) {
        sp_rt_release(ptr);
        return;
    }
    std::free(ptr);
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
        // Parent fix (Phase 37 merge): unregister the payload before
        // freeing the header (see get_arc_header).
        {
            std::lock_guard<std::mutex> lock(g_arc_registry_mutex);
            g_arc_payloads.erase(reinterpret_cast<uint8_t*>(header) + sizeof(SPObjectHeader));
        }
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
    // Phase 37 slice B: element stride for the destructor, and
    // whether the stored elements are retained references (set by
    // sp_rt_array_mark_ref_elements). The data is inline after the
    // header, so sizeof() is the only layout dependency.
    int64_t element_size;
    bool elements_are_refs;
};

// Phase 37 slice B: ARC destructors for the containers. A destructor
// frees the container's INTERNAL buffers only — never the impl struct
// itself (the ARC machinery frees the payload). It releases stored
// elements only when the container's flag is set; an unflagged
// container degrades to the old behaviour (storage freed, elements
// untouched). sp_rt_release is null-safe and magic-checked, so
// releasing a null or non-ARC element is a no-op.
static void sp_array_destructor(void* ptr) {
    auto* header = static_cast<SPArrayHeader*>(ptr);
    if (header->elements_are_refs) {
        int64_t es = header->element_size > 0 ? header->element_size : 8;
        auto* data = reinterpret_cast<uint8_t*>(ptr) + sizeof(SPArrayHeader);
        for (int64_t i = 0; i < header->length; ++i) {
            void* elem = nullptr;
            std::memcpy(&elem, data + i * es, sizeof(void*));
            sp_rt_release(elem);
        }
    }
}

// ---------------------------------------------------------------------------
// Phase 13: Pure Native Collection Primitives (Zero C++ STL)
// ---------------------------------------------------------------------------

struct SPListImpl {
    int64_t element_size;
    int64_t length;
    int64_t capacity;
    int64_t* items;
    // Phase 37 slice B: whether the stored elements are retained
    // references (set by sp_rt_list_mark_ref_elements).
    bool elements_are_refs;
};

static void sp_list_destructor(void* ptr) {
    auto* impl = static_cast<SPListImpl*>(ptr);
    if (impl->elements_are_refs && impl->items) {
        for (int64_t i = 0; i < impl->length; ++i) {
            sp_rt_release(reinterpret_cast<void*>(impl->items[i]));
        }
    }
    sp_rt_free(impl->items);
}

static inline uint32_t sp_fnv1a(const char* s) {
    uint32_t h = 2166136261u;
    while (s && *s) {
        h ^= static_cast<uint8_t>(*s++);
        h *= 16777619u;
    }
    return h;
}

static inline uint32_t sp_hash_int(int64_t key) {
    uint64_t k = static_cast<uint64_t>(key) * 11400714819323198485ULL;
    return static_cast<uint32_t>(k ^ (k >> 32));
}

struct SPMapStrEntry {
    char* key;
    int64_t val;
    uint32_t hash;
    bool occupied;
};

struct SPMapIntEntry {
    int64_t key;
    int64_t val;
    uint32_t hash;
    bool occupied;
};

struct SPMapImpl {
    SPMapStrEntry* str_entries;
    int64_t str_capacity;
    int64_t str_count;

    SPMapIntEntry* int_entries;
    int64_t int_capacity;
    int64_t int_count;

    // Phase 37 slice B: whether the stored VALUES are retained
    // references (Map semantics, set by
    // sp_rt_map_mark_ref_elements), and whether the stored elements
    // are the INT-TABLE KEYS (Set semantics, set by
    // sp_rt_set_mark_ref_elements — a set stores its element as the
    // key with a dummy value 1). The two are mutually exclusive.
    bool elements_are_refs;
    bool ref_elements_are_keys;
};

static void sp_map_destructor(void* ptr) {
    auto* impl = static_cast<SPMapImpl*>(ptr);
    if (impl->elements_are_refs) {
        if (impl->str_entries) {
            for (int64_t i = 0; i < impl->str_capacity; ++i) {
                if (impl->str_entries[i].occupied) {
                    sp_rt_release(reinterpret_cast<void*>(impl->str_entries[i].val));
                }
            }
        }
        if (impl->int_entries) {
            for (int64_t i = 0; i < impl->int_capacity; ++i) {
                if (impl->int_entries[i].occupied) {
                    sp_rt_release(reinterpret_cast<void*>(impl->int_entries[i].val));
                }
            }
        }
    }
    if (impl->str_entries) {
        for (int64_t i = 0; i < impl->str_capacity; ++i) {
            if (impl->str_entries[i].occupied) {
                sp_rt_free(impl->str_entries[i].key);
            }
        }
        sp_rt_free(impl->str_entries);
    }
    sp_rt_free(impl->int_entries);
}

struct SPSetImpl {
    SPMapImpl map;
};

// Phase 37 slice B: a set's elements are the int-table keys (and the
// str-table keys are strdup'd string copies). The destructor releases
// the int keys when flagged and frees the str keys; it never releases
// the dummy values (always 1).
static void sp_set_destructor(void* ptr) {
    auto* impl = &static_cast<SPSetImpl*>(ptr)->map;
    if (impl->ref_elements_are_keys && impl->int_entries) {
        for (int64_t i = 0; i < impl->int_capacity; ++i) {
            if (impl->int_entries[i].occupied) {
                sp_rt_release(reinterpret_cast<void*>(impl->int_entries[i].key));
            }
        }
    }
    if (impl->str_entries) {
        for (int64_t i = 0; i < impl->str_capacity; ++i) {
            if (impl->str_entries[i].occupied) {
                sp_rt_free(impl->str_entries[i].key);
            }
        }
        sp_rt_free(impl->str_entries);
    }
    sp_rt_free(impl->int_entries);
}

// Native Array primitives
// Phase 37 slice B: ARC-allocated (sp_rt_arc_alloc) so a scope drop
// frees the array and, when flagged, releases its elements. The
// payload is the header + inline data; sp_rt_arc_alloc memsets it,
// so elements_are_refs starts false.
void* sp_rt_array_alloc(int64_t length, int64_t element_size) {
    if (length < 0) length = 0;
    if (element_size <= 0) element_size = 8;
    size_t total_size = sizeof(SPArrayHeader) + static_cast<size_t>(length * element_size);
    auto* header = static_cast<SPArrayHeader*>(sp_rt_arc_alloc(total_size, sp_array_destructor));
    header->length = length;
    header->capacity = length;
    header->element_size = element_size;
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
    // Phase 37 slice B: the overwritten element's share is released
    // (the compiler retained the new value before this call, so
    // overwriting a slot with the same object nets to no change).
    auto* header = static_cast<SPArrayHeader*>(arr);
    int64_t old = *reinterpret_cast<int64_t*>(ptr);
    *reinterpret_cast<int64_t*>(ptr) = val;
    if (header->elements_are_refs) {
        sp_rt_release(reinterpret_cast<void*>(old));
    }
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
// Phase 37 slice B: ARC-allocated; the impl struct is POD, so the
// arc_alloc memset is a correct initialization (elements_are_refs
// starts false). sp_rt_stack_alloc delegates here, so Stack gets the
// List destructor for free.
void* sp_rt_list_alloc(int64_t element_size, int64_t initial_capacity) {
    auto* list = static_cast<SPListImpl*>(sp_rt_arc_alloc(sizeof(SPListImpl), sp_list_destructor));
    list->element_size = element_size > 0 ? element_size : 8;
    list->capacity = initial_capacity > 0 ? initial_capacity : 8;
    list->length = 0;
    list->items = static_cast<int64_t*>(sp_rt_alloc(list->capacity * sizeof(int64_t)));
    return list;
}

int64_t sp_rt_list_size(void* list) {
    if (!list) return 0;
    return static_cast<SPListImpl*>(list)->length;
}

void sp_rt_list_push_i64(void* list, int64_t val) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    auto* impl = static_cast<SPListImpl*>(list);
    if (impl->length >= impl->capacity) {
        int64_t new_cap = impl->capacity < 8 ? 8 : impl->capacity * 2;
        auto* new_items = static_cast<int64_t*>(sp_rt_alloc(new_cap * sizeof(int64_t)));
        if (impl->length > 0 && impl->items) {
            std::memcpy(new_items, impl->items, impl->length * sizeof(int64_t));
            sp_rt_free(impl->items);
        }
        impl->items = new_items;
        impl->capacity = new_cap;
    }
    impl->items[impl->length++] = val;
}

int64_t sp_rt_list_get_i64(void* list, int64_t index) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    auto* impl = static_cast<SPListImpl*>(list);
    if (index < 0 || index >= impl->length) {
        sp_rt_panic("IndexOutOfBoundsException: index out of bounds in list.get");
    }
    return impl->items[index];
}

void sp_rt_list_set_i64(void* list, int64_t index, int64_t val) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    auto* impl = static_cast<SPListImpl*>(list);
    if (index < 0 || index >= impl->length) {
        sp_rt_panic("IndexOutOfBoundsException: index out of bounds in list.set");
    }
    // Phase 37 slice B: release the overwritten element's share (see
    // sp_rt_array_set_i64 for the retain-first ordering note).
    int64_t old = impl->items[index];
    impl->items[index] = val;
    if (impl->elements_are_refs) {
        sp_rt_release(reinterpret_cast<void*>(old));
    }
}

int64_t sp_rt_list_pop_i64(void* list) {
    if (!list) sp_rt_panic("NullPointerException: list is null");
    auto* impl = static_cast<SPListImpl*>(list);
    if (impl->length <= 0) {
        sp_rt_panic("NoSuchElementException: cannot pop from empty list");
    }
    return impl->items[--impl->length];
}

void sp_rt_list_clear(void* list) {
    if (!list) return;
    // Phase 37 slice B: a cleared list gives up every element share.
    auto* impl = static_cast<SPListImpl*>(list);
    if (impl->elements_are_refs && impl->items) {
        for (int64_t i = 0; i < impl->length; ++i) {
            sp_rt_release(reinterpret_cast<void*>(impl->items[i]));
        }
    }
    impl->length = 0;
}

bool sp_rt_list_contains_i64(void* list, int64_t val) {
    if (!list) return false;
    auto* impl = static_cast<SPListImpl*>(list);
    for (int64_t i = 0; i < impl->length; ++i) {
        if (impl->items[i] == val) return true;
    }
    return false;
}

void sp_rt_list_remove_at(void* list, int64_t index) {
    if (!list) return;
    auto* impl = static_cast<SPListImpl*>(list);
    if (index >= 0 && index < impl->length) {
        // Phase 37 slice B: the removed element's share is released.
        // (pop is a TRANSFER and must not release — see sp_rt_list_pop_i64.)
        int64_t removed = impl->items[index];
        if (index < impl->length - 1) {
            std::memmove(&impl->items[index], &impl->items[index + 1],
                         (impl->length - index - 1) * sizeof(int64_t));
        }
        impl->length--;
        if (impl->elements_are_refs) {
            sp_rt_release(reinterpret_cast<void*>(removed));
        }
    }
}

bool sp_rt_list_is_empty(void* list) {
    if (!list) return true;
    return static_cast<SPListImpl*>(list)->length == 0;
}

// Native Map<K, V> primitives
// Phase 37 slice B: ARC-allocated (see sp_rt_list_alloc).
void* sp_rt_map_alloc() {
    auto* map = static_cast<SPMapImpl*>(sp_rt_arc_alloc(sizeof(SPMapImpl), sp_map_destructor));
    map->str_capacity = 16;
    map->str_count = 0;
    map->str_entries = static_cast<SPMapStrEntry*>(sp_rt_alloc(16 * sizeof(SPMapStrEntry)));

    map->int_capacity = 16;
    map->int_count = 0;
    map->int_entries = static_cast<SPMapIntEntry*>(sp_rt_alloc(16 * sizeof(SPMapIntEntry)));
    return map;
}

int64_t sp_rt_map_size(void* map) {
    if (!map) return 0;
    auto* impl = static_cast<SPMapImpl*>(map);
    return impl->str_count + impl->int_count;
}

// Phase 6 slice A: key access by ordinal for `for (key in map)`.
// Returns the key of the idx-th occupied entry in table order. The
// order is unspecified and changes if the table rehashes, so the map
// must not be mutated while it is being iterated. Out of range (or a
// null map) returns null / 0.
const char* sp_rt_map_str_key_at(void* map, int64_t index) {
    if (!map || index < 0) return nullptr;
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->str_entries || impl->str_capacity == 0) return nullptr;
    int64_t seen = 0;
    for (int64_t i = 0; i < impl->str_capacity; ++i) {
        if (impl->str_entries[i].occupied) {
            if (seen == index) return impl->str_entries[i].key;
            ++seen;
        }
    }
    return nullptr;
}

int64_t sp_rt_map_int_key_at(void* map, int64_t index) {
    if (!map || index < 0) return 0;
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->int_entries || impl->int_capacity == 0) return 0;
    int64_t seen = 0;
    for (int64_t i = 0; i < impl->int_capacity; ++i) {
        if (impl->int_entries[i].occupied) {
            if (seen == index) return impl->int_entries[i].key;
            ++seen;
        }
    }
    return 0;
}

static void sp_map_rehash_str(SPMapImpl* impl, int64_t new_cap) {
    auto* old_entries = impl->str_entries;
    int64_t old_cap = impl->str_capacity;

    impl->str_entries = static_cast<SPMapStrEntry*>(sp_rt_alloc(new_cap * sizeof(SPMapStrEntry)));
    impl->str_capacity = new_cap;
    impl->str_count = 0;

    for (int64_t i = 0; i < old_cap; ++i) {
        if (old_entries[i].occupied) {
            uint32_t h = old_entries[i].hash;
            int64_t idx = h & (new_cap - 1);
            while (impl->str_entries[idx].occupied) {
                idx = (idx + 1) & (new_cap - 1);
            }
            impl->str_entries[idx] = old_entries[i];
            impl->str_count++;
        }
    }
    sp_rt_free(old_entries);
}

void sp_rt_map_set_str(void* map, const char* key, int64_t val) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    if (!key) return;
    auto* impl = static_cast<SPMapImpl*>(map);

    if (impl->str_count * 10 >= impl->str_capacity * 7) {
        sp_map_rehash_str(impl, impl->str_capacity * 2);
    }

    uint32_t h = sp_fnv1a(key);
    int64_t idx = h & (impl->str_capacity - 1);
    while (impl->str_entries[idx].occupied) {
        if (impl->str_entries[idx].hash == h && std::strcmp(impl->str_entries[idx].key, key) == 0) {
            // Phase 37 slice B: overwriting a value releases the old
            // value's share (the compiler retained the new value
            // before this call; same object nets to no change). The
            // key copy is untouched — the key is not the element.
            int64_t old = impl->str_entries[idx].val;
            impl->str_entries[idx].val = val;
            if (impl->elements_are_refs) {
                sp_rt_release(reinterpret_cast<void*>(old));
            }
            return;
        }
        idx = (idx + 1) & (impl->str_capacity - 1);
    }

    size_t len = std::strlen(key);
    char* copy = static_cast<char*>(sp_rt_alloc(len + 1));
    std::memcpy(copy, key, len + 1);

    impl->str_entries[idx].key = copy;
    impl->str_entries[idx].val = val;
    impl->str_entries[idx].hash = h;
    impl->str_entries[idx].occupied = true;
    impl->str_count++;
}

int64_t sp_rt_map_get_str(void* map, const char* key) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    if (!key) return 0;
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->str_entries || impl->str_capacity == 0) return 0;

    uint32_t h = sp_fnv1a(key);
    int64_t idx = h & (impl->str_capacity - 1);
    while (impl->str_entries[idx].occupied) {
        if (impl->str_entries[idx].hash == h && std::strcmp(impl->str_entries[idx].key, key) == 0) {
            return impl->str_entries[idx].val;
        }
        idx = (idx + 1) & (impl->str_capacity - 1);
    }
    return 0;
}

bool sp_rt_map_contains_str(void* map, const char* key) {
    if (!map || !key) return false;
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->str_entries || impl->str_capacity == 0) return false;

    uint32_t h = sp_fnv1a(key);
    int64_t idx = h & (impl->str_capacity - 1);
    while (impl->str_entries[idx].occupied) {
        if (impl->str_entries[idx].hash == h && std::strcmp(impl->str_entries[idx].key, key) == 0) {
            return true;
        }
        idx = (idx + 1) & (impl->str_capacity - 1);
    }
    return false;
}

void sp_rt_map_remove_str(void* map, const char* key) {
    if (!map || !key) return;
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->str_entries || impl->str_capacity == 0) return;

    uint32_t h = sp_fnv1a(key);
    int64_t idx = h & (impl->str_capacity - 1);
    while (impl->str_entries[idx].occupied) {
        if (impl->str_entries[idx].hash == h && std::strcmp(impl->str_entries[idx].key, key) == 0) {
            // Phase 37 slice B: the removed value's share is released
            // (captured before the backward-shift deletion moves the
            // entry). The key copy is freed as before; a set's str key
            // is a string copy, never a reference.
            int64_t removed_val = impl->str_entries[idx].val;
            sp_rt_free(impl->str_entries[idx].key);
            impl->str_count--;

            int64_t i = idx;
            int64_t j = (i + 1) & (impl->str_capacity - 1);
            while (impl->str_entries[j].occupied) {
                int64_t k = impl->str_entries[j].hash & (impl->str_capacity - 1);
                bool should_move = (i <= j) ? (k <= i || k > j) : (k <= i && k > j);
                if (should_move) {
                    impl->str_entries[i] = impl->str_entries[j];
                    i = j;
                }
                j = (j + 1) & (impl->str_capacity - 1);
            }
            impl->str_entries[i].occupied = false;
            impl->str_entries[i].key = nullptr;
            impl->str_entries[i].val = 0;
            impl->str_entries[i].hash = 0;
            if (impl->elements_are_refs) {
                sp_rt_release(reinterpret_cast<void*>(removed_val));
            }
            return;
        }
        idx = (idx + 1) & (impl->str_capacity - 1);
    }
}

static void sp_map_rehash_int(SPMapImpl* impl, int64_t new_cap) {
    auto* old_entries = impl->int_entries;
    int64_t old_cap = impl->int_capacity;

    impl->int_entries = static_cast<SPMapIntEntry*>(sp_rt_alloc(new_cap * sizeof(SPMapIntEntry)));
    impl->int_capacity = new_cap;
    impl->int_count = 0;

    for (int64_t i = 0; i < old_cap; ++i) {
        if (old_entries[i].occupied) {
            uint32_t h = old_entries[i].hash;
            int64_t idx = h & (new_cap - 1);
            while (impl->int_entries[idx].occupied) {
                idx = (idx + 1) & (new_cap - 1);
            }
            impl->int_entries[idx] = old_entries[i];
            impl->int_count++;
        }
    }
    sp_rt_free(old_entries);
}

void sp_rt_map_set_i64(void* map, int64_t key, int64_t val) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    auto* impl = static_cast<SPMapImpl*>(map);

    if (impl->int_count * 10 >= impl->int_capacity * 7) {
        sp_map_rehash_int(impl, impl->int_capacity * 2);
    }

    uint32_t h = sp_hash_int(key);
    int64_t idx = h & (impl->int_capacity - 1);
    while (impl->int_entries[idx].occupied) {
        if (impl->int_entries[idx].hash == h && impl->int_entries[idx].key == key) {
            // Phase 37 slice B: overwriting a value releases the old
            // value's share (see sp_rt_map_set_str). For a Set's
            // backing map the value is the dummy 1 and
            // elements_are_refs is false, so a duplicate add releases
            // nothing; the element (the key) is untouched.
            int64_t old = impl->int_entries[idx].val;
            impl->int_entries[idx].val = val;
            if (impl->elements_are_refs) {
                sp_rt_release(reinterpret_cast<void*>(old));
            }
            return;
        }
        idx = (idx + 1) & (impl->int_capacity - 1);
    }

    impl->int_entries[idx].key = key;
    impl->int_entries[idx].val = val;
    impl->int_entries[idx].hash = h;
    impl->int_entries[idx].occupied = true;
    impl->int_count++;
}

int64_t sp_rt_map_get_i64(void* map, int64_t key) {
    if (!map) sp_rt_panic("NullPointerException: map is null");
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->int_entries || impl->int_capacity == 0) return 0;

    uint32_t h = sp_hash_int(key);
    int64_t idx = h & (impl->int_capacity - 1);
    while (impl->int_entries[idx].occupied) {
        if (impl->int_entries[idx].hash == h && impl->int_entries[idx].key == key) {
            return impl->int_entries[idx].val;
        }
        idx = (idx + 1) & (impl->int_capacity - 1);
    }
    return 0;
}

bool sp_rt_map_contains_i64(void* map, int64_t key) {
    if (!map) return false;
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->int_entries || impl->int_capacity == 0) return false;

    uint32_t h = sp_hash_int(key);
    int64_t idx = h & (impl->int_capacity - 1);
    while (impl->int_entries[idx].occupied) {
        if (impl->int_entries[idx].hash == h && impl->int_entries[idx].key == key) {
            return true;
        }
        idx = (idx + 1) & (impl->int_capacity - 1);
    }
    return false;
}

void sp_rt_map_remove_i64(void* map, int64_t key) {
    if (!map) return;
    auto* impl = static_cast<SPMapImpl*>(map);
    if (!impl->int_entries || impl->int_capacity == 0) return;

    uint32_t h = sp_hash_int(key);
    int64_t idx = h & (impl->int_capacity - 1);
    while (impl->int_entries[idx].occupied) {
        if (impl->int_entries[idx].hash == h && impl->int_entries[idx].key == key) {
            // Phase 37 slice B: the removed entry's share is released
            // (captured before the backward-shift deletion moves the
            // entry). For a Map the share is the VALUE; for a Set's
            // backing map the share is the KEY (the element) and the
            // value is the dummy 1, never released.
            int64_t removed_val = impl->int_entries[idx].val;
            int64_t removed_key = impl->int_entries[idx].key;
            impl->int_count--;

            int64_t i = idx;
            int64_t j = (i + 1) & (impl->int_capacity - 1);
            while (impl->int_entries[j].occupied) {
                int64_t k = impl->int_entries[j].hash & (impl->int_capacity - 1);
                bool should_move = (i <= j) ? (k <= i || k > j) : (k <= i && k > j);
                if (should_move) {
                    impl->int_entries[i] = impl->int_entries[j];
                    i = j;
                }
                j = (j + 1) & (impl->int_capacity - 1);
            }
            impl->int_entries[i].occupied = false;
            impl->int_entries[i].key = 0;
            impl->int_entries[i].val = 0;
            impl->int_entries[i].hash = 0;
            if (impl->elements_are_refs) {
                sp_rt_release(reinterpret_cast<void*>(removed_val));
            }
            if (impl->ref_elements_are_keys) {
                sp_rt_release(reinterpret_cast<void*>(removed_key));
            }
            return;
        }
        idx = (idx + 1) & (impl->int_capacity - 1);
    }
}

void sp_rt_map_clear(void* map) {
    if (!map) return;
    auto* impl = static_cast<SPMapImpl*>(map);
    // Phase 37 slice B: a cleared map gives up every element share
    // (values for a Map; int keys for a Set's backing map) before the
    // tables are zeroed.
    if (impl->elements_are_refs) {
        for (int64_t i = 0; i < impl->str_capacity; ++i) {
            if (impl->str_entries[i].occupied) {
                sp_rt_release(reinterpret_cast<void*>(impl->str_entries[i].val));
            }
        }
        for (int64_t i = 0; i < impl->int_capacity; ++i) {
            if (impl->int_entries[i].occupied) {
                sp_rt_release(reinterpret_cast<void*>(impl->int_entries[i].val));
            }
        }
    }
    if (impl->ref_elements_are_keys) {
        for (int64_t i = 0; i < impl->int_capacity; ++i) {
            if (impl->int_entries[i].occupied) {
                sp_rt_release(reinterpret_cast<void*>(impl->int_entries[i].key));
            }
        }
    }
    for (int64_t i = 0; i < impl->str_capacity; ++i) {
        if (impl->str_entries[i].occupied) {
            sp_rt_free(impl->str_entries[i].key);
        }
    }
    std::memset(impl->str_entries, 0, impl->str_capacity * sizeof(SPMapStrEntry));
    impl->str_count = 0;

    std::memset(impl->int_entries, 0, impl->int_capacity * sizeof(SPMapIntEntry));
    impl->int_count = 0;
}

bool sp_rt_map_is_empty(void* map) {
    if (!map) return true;
    auto* impl = static_cast<SPMapImpl*>(map);
    return (impl->str_count + impl->int_count) == 0;
}

// Native Set<T> primitives
// Phase 37 slice B: ARC-allocated (see sp_rt_list_alloc).
void* sp_rt_set_alloc() {
    auto* set = static_cast<SPSetImpl*>(sp_rt_arc_alloc(sizeof(SPSetImpl), sp_set_destructor));
    set->map.str_capacity = 16;
    set->map.str_count = 0;
    set->map.str_entries = static_cast<SPMapStrEntry*>(sp_rt_alloc(16 * sizeof(SPMapStrEntry)));

    set->map.int_capacity = 16;
    set->map.int_count = 0;
    set->map.int_entries = static_cast<SPMapIntEntry*>(sp_rt_alloc(16 * sizeof(SPMapIntEntry)));
    return set;
}

int64_t sp_rt_set_size(void* set) {
    if (!set) return 0;
    return sp_rt_map_size(&static_cast<SPSetImpl*>(set)->map);
}

void sp_rt_set_add_i64(void* set, int64_t val) {
    if (!set) sp_rt_panic("NullPointerException: set is null");
    sp_rt_map_set_i64(&static_cast<SPSetImpl*>(set)->map, val, 1);
}

void sp_rt_set_add_str(void* set, const char* val) {
    if (!set) sp_rt_panic("NullPointerException: set is null");
    sp_rt_map_set_str(&static_cast<SPSetImpl*>(set)->map, val, 1);
}

bool sp_rt_set_contains_i64(void* set, int64_t val) {
    if (!set) return false;
    return sp_rt_map_contains_i64(&static_cast<SPSetImpl*>(set)->map, val);
}

bool sp_rt_set_contains_str(void* set, const char* val) {
    if (!set || !val) return false;
    return sp_rt_map_contains_str(&static_cast<SPSetImpl*>(set)->map, val);
}

void sp_rt_set_remove_i64(void* set, int64_t val) {
    if (!set) return;
    sp_rt_map_remove_i64(&static_cast<SPSetImpl*>(set)->map, val);
}

void sp_rt_set_remove_str(void* set, const char* val) {
    if (!set || !val) return;
    sp_rt_map_remove_str(&static_cast<SPSetImpl*>(set)->map, val);
}

void sp_rt_set_clear(void* set) {
    if (!set) return;
    sp_rt_map_clear(&static_cast<SPSetImpl*>(set)->map);
}

bool sp_rt_set_is_empty(void* set) {
    if (!set) return true;
    return sp_rt_map_is_empty(&static_cast<SPSetImpl*>(set)->map);
}

// Phase 6 slice B: indexed access into a set, for `for (x in set)`.
// A set is backed by the map's entry arrays; entries are visited in
// table order (hash order — iteration order is unspecified, and a set
// must not be mutated during iteration). Out-of-range indices panic:
// the for-in loop only ever asks for indices below sp_rt_set_size().
int64_t sp_rt_set_get_i64(void* set, int64_t index) {
    if (!set) sp_rt_panic("NullPointerException: set is null");
    auto* impl = &static_cast<SPSetImpl*>(set)->map;
    int64_t seen = 0;
    for (int64_t i = 0; i < impl->int_capacity; ++i) {
        if (impl->int_entries[i].occupied) {
            if (seen == index) return impl->int_entries[i].key;
            ++seen;
        }
    }
    sp_rt_panic("Set index out of range");
    return 0;
}

const char* sp_rt_set_get_str(void* set, int64_t index) {
    if (!set) sp_rt_panic("NullPointerException: set is null");
    auto* impl = &static_cast<SPSetImpl*>(set)->map;
    int64_t seen = 0;
    for (int64_t i = 0; i < impl->str_capacity; ++i) {
        if (impl->str_entries[i].occupied) {
            if (seen == index) return impl->str_entries[i].key;
            ++seen;
        }
    }
    sp_rt_panic("Set index out of range");
    return "";
}

// Phase 6 slice B: printable form of a set, for string interpolation
// ("${s}" used to print raw pointer bytes — any pointer value was
// assumed to be a C string). Integer entries first, then string
// entries, both in table order: {1, 2, 3} / {a, b} / {} when empty.
const char* sp_rt_set_to_string(void* set) {
    if (!set) return "{}";
    auto* impl = &static_cast<SPSetImpl*>(set)->map;
    std::string out = "{";
    bool first = true;
    for (int64_t i = 0; i < impl->int_capacity; ++i) {
        if (impl->int_entries[i].occupied) {
            if (!first) out += ", ";
            out += std::to_string(impl->int_entries[i].key);
            first = false;
        }
    }
    for (int64_t i = 0; i < impl->str_capacity; ++i) {
        if (impl->str_entries[i].occupied) {
            if (!first) out += ", ";
            out += impl->str_entries[i].key ? impl->str_entries[i].key : "";
            first = false;
        }
    }
    out += "}";
    char* res = static_cast<char*>(malloc(out.size() + 1));
    if (res) std::memcpy(res, out.c_str(), out.size() + 1);
    return res;
}

bool sp_rt_array_is_empty(void* arr) {
    if (!arr) return true;
    return sp_rt_array_length(arr) == 0;
}

// Native Stack<T> primitives
void* sp_rt_stack_alloc() {
    return sp_rt_list_alloc(8, 16);
}

void sp_rt_stack_push(void* stack, int64_t val) {
    sp_rt_list_push_i64(stack, val);
}

int64_t sp_rt_stack_pop(void* stack) {
    if (!stack || sp_rt_list_size(stack) == 0) {
        sp_rt_panic("IllegalStateException: stack is empty");
    }
    return sp_rt_list_pop_i64(stack);
}

int64_t sp_rt_stack_peek(void* stack) {
    if (!stack || sp_rt_list_size(stack) == 0) {
        sp_rt_panic("IllegalStateException: stack is empty");
    }
    return sp_rt_list_get_i64(stack, sp_rt_list_size(stack) - 1);
}

int64_t sp_rt_stack_size(void* stack) {
    return sp_rt_list_size(stack);
}

bool sp_rt_stack_is_empty(void* stack) {
    return sp_rt_list_is_empty(stack);
}

void sp_rt_stack_clear(void* stack) {
    sp_rt_list_clear(stack);
}

// Native Queue<T> primitives
struct SPQueueImpl {
    int64_t* items;
    int64_t head;
    int64_t count;
    int64_t capacity;
    // Phase 37 slice B: whether the queued items are retained
    // references (set by sp_rt_queue_mark_ref_elements).
    bool elements_are_refs;
};

// Phase 37 slice B: release the queued items (ring-buffer order)
// when flagged, then free the ring buffer. The impl struct itself is
// freed by the ARC machinery, never here.
static void sp_queue_destructor(void* ptr) {
    auto* q = static_cast<SPQueueImpl*>(ptr);
    if (q->elements_are_refs && q->items && q->capacity > 0) {
        for (int64_t i = 0; i < q->count; ++i) {
            sp_rt_release(reinterpret_cast<void*>(q->items[(q->head + i) % q->capacity]));
        }
    }
    sp_rt_free(q->items);
}

void* sp_rt_queue_alloc() {
    auto* q = static_cast<SPQueueImpl*>(sp_rt_arc_alloc(sizeof(SPQueueImpl), sp_queue_destructor));
    q->capacity = 16;
    q->head = 0;
    q->count = 0;
    q->items = static_cast<int64_t*>(sp_rt_alloc(q->capacity * sizeof(int64_t)));
    return q;
}

void sp_rt_queue_enqueue(void* queue, int64_t val) {
    if (!queue) sp_rt_panic("NullPointerException: queue is null");
    auto* q = static_cast<SPQueueImpl*>(queue);
    if (q->head + q->count >= q->capacity) {
        if (q->head > 0) {
            std::memmove(q->items, q->items + q->head, q->count * sizeof(int64_t));
            q->head = 0;
        } else {
            int64_t new_cap = q->capacity * 2;
            auto* new_items = static_cast<int64_t*>(sp_rt_alloc(new_cap * sizeof(int64_t)));
            std::memcpy(new_items, q->items, q->count * sizeof(int64_t));
            sp_rt_free(q->items);
            q->items = new_items;
            q->capacity = new_cap;
        }
    }
    q->items[q->head + q->count] = val;
    q->count++;
}

int64_t sp_rt_queue_dequeue(void* queue) {
    if (!queue) sp_rt_panic("NullPointerException: queue is null");
    auto* q = static_cast<SPQueueImpl*>(queue);
    if (q->count == 0) {
        sp_rt_panic("IllegalStateException: queue is empty");
    }
    int64_t val = q->items[q->head];
    q->head++;
    q->count--;
    return val;
}

int64_t sp_rt_queue_peek(void* queue) {
    if (!queue) sp_rt_panic("NullPointerException: queue is null");
    auto* q = static_cast<SPQueueImpl*>(queue);
    if (q->count == 0) {
        sp_rt_panic("IllegalStateException: queue is empty");
    }
    return q->items[q->head];
}

int64_t sp_rt_queue_size(void* queue) {
    if (!queue) return 0;
    return static_cast<SPQueueImpl*>(queue)->count;
}

bool sp_rt_queue_is_empty(void* queue) {
    if (!queue) return true;
    return static_cast<SPQueueImpl*>(queue)->count == 0;
}

void sp_rt_queue_clear(void* queue) {
    if (!queue) return;
    auto* q = static_cast<SPQueueImpl*>(queue);
    // Phase 37 slice B: a cleared queue gives up every queued item's
    // share.
    if (q->elements_are_refs && q->items && q->capacity > 0) {
        for (int64_t i = 0; i < q->count; ++i) {
            sp_rt_release(reinterpret_cast<void*>(q->items[(q->head + i) % q->capacity]));
        }
    }
    q->head = 0;
    q->count = 0;
}

// Native PriorityQueue<T> primitives (binary min-heap)
struct SPPQImpl {
    int64_t* items;
    int64_t* priorities;
    int64_t count;
    int64_t capacity;
    // Phase 37 slice B: whether the stored items are retained
    // references (set by sp_rt_pq_mark_ref_elements).
    bool elements_are_refs;
};

// Phase 37 slice B: release the stored items when flagged, then free
// both buffers. The impl struct itself is freed by the ARC
// machinery, never here.
static void sp_pq_destructor(void* ptr) {
    auto* pq = static_cast<SPPQImpl*>(ptr);
    if (pq->elements_are_refs && pq->items) {
        for (int64_t i = 0; i < pq->count; ++i) {
            sp_rt_release(reinterpret_cast<void*>(pq->items[i]));
        }
    }
    sp_rt_free(pq->items);
    sp_rt_free(pq->priorities);
}

void* sp_rt_pq_alloc() {
    auto* pq = static_cast<SPPQImpl*>(sp_rt_arc_alloc(sizeof(SPPQImpl), sp_pq_destructor));
    pq->capacity = 16;
    pq->count = 0;
    pq->items = static_cast<int64_t*>(sp_rt_alloc(pq->capacity * sizeof(int64_t)));
    pq->priorities = static_cast<int64_t*>(sp_rt_alloc(pq->capacity * sizeof(int64_t)));
    return pq;
}

static void sp_pq_swap(SPPQImpl* pq, int64_t i, int64_t j) {
    int64_t t_item = pq->items[i];
    int64_t t_prio = pq->priorities[i];
    pq->items[i] = pq->items[j];
    pq->priorities[i] = pq->priorities[j];
    pq->items[j] = t_item;
    pq->priorities[j] = t_prio;
}

static void sp_pq_sift_up(SPPQImpl* pq, int64_t idx) {
    while (idx > 0) {
        int64_t parent = (idx - 1) / 2;
        if (pq->priorities[idx] < pq->priorities[parent]) {
            sp_pq_swap(pq, idx, parent);
            idx = parent;
        } else {
            break;
        }
    }
}

static void sp_pq_sift_down(SPPQImpl* pq, int64_t idx) {
    while (true) {
        int64_t left = 2 * idx + 1;
        int64_t right = 2 * idx + 2;
        int64_t smallest = idx;
        if (left < pq->count && pq->priorities[left] < pq->priorities[smallest]) {
            smallest = left;
        }
        if (right < pq->count && pq->priorities[right] < pq->priorities[smallest]) {
            smallest = right;
        }
        if (smallest != idx) {
            sp_pq_swap(pq, idx, smallest);
            idx = smallest;
        } else {
            break;
        }
    }
}

void sp_rt_pq_push(void* pq_ptr, int64_t val, int64_t priority) {
    if (!pq_ptr) sp_rt_panic("NullPointerException: priority queue is null");
    auto* pq = static_cast<SPPQImpl*>(pq_ptr);
    if (pq->count >= pq->capacity) {
        int64_t new_cap = pq->capacity * 2;
        auto* new_items = static_cast<int64_t*>(sp_rt_alloc(new_cap * sizeof(int64_t)));
        auto* new_prios = static_cast<int64_t*>(sp_rt_alloc(new_cap * sizeof(int64_t)));
        std::memcpy(new_items, pq->items, pq->count * sizeof(int64_t));
        std::memcpy(new_prios, pq->priorities, pq->count * sizeof(int64_t));
        sp_rt_free(pq->items);
        sp_rt_free(pq->priorities);
        pq->items = new_items;
        pq->priorities = new_prios;
        pq->capacity = new_cap;
    }
    pq->items[pq->count] = val;
    pq->priorities[pq->count] = priority;
    pq->count++;
    sp_pq_sift_up(pq, pq->count - 1);
}

int64_t sp_rt_pq_pop(void* pq_ptr) {
    if (!pq_ptr) sp_rt_panic("NullPointerException: priority queue is null");
    auto* pq = static_cast<SPPQImpl*>(pq_ptr);
    if (pq->count == 0) {
        sp_rt_panic("IllegalStateException: priority queue is empty");
    }
    int64_t root = pq->items[0];
    pq->count--;
    if (pq->count > 0) {
        pq->items[0] = pq->items[pq->count];
        pq->priorities[0] = pq->priorities[pq->count];
        sp_pq_sift_down(pq, 0);
    }
    return root;
}

int64_t sp_rt_pq_peek(void* pq_ptr) {
    if (!pq_ptr) sp_rt_panic("NullPointerException: priority queue is null");
    auto* pq = static_cast<SPPQImpl*>(pq_ptr);
    if (pq->count == 0) {
        sp_rt_panic("IllegalStateException: priority queue is empty");
    }
    return pq->items[0];
}

int64_t sp_rt_pq_peek_priority(void* pq_ptr) {
    if (!pq_ptr) sp_rt_panic("NullPointerException: priority queue is null");
    auto* pq = static_cast<SPPQImpl*>(pq_ptr);
    if (pq->count == 0) {
        sp_rt_panic("IllegalStateException: priority queue is empty");
    }
    return pq->priorities[0];
}

int64_t sp_rt_pq_size(void* pq_ptr) {
    if (!pq_ptr) return 0;
    return static_cast<SPPQImpl*>(pq_ptr)->count;
}

bool sp_rt_pq_is_empty(void* pq_ptr) {
    if (!pq_ptr) return true;
    return static_cast<SPPQImpl*>(pq_ptr)->count == 0;
}

void sp_rt_pq_clear(void* pq_ptr) {
    if (!pq_ptr) return;
    auto* pq = static_cast<SPPQImpl*>(pq_ptr);
    if (pq->elements_are_refs && pq->items) {
        for (int64_t i = 0; i < pq->count; ++i) {
            sp_rt_release(reinterpret_cast<void*>(pq->items[i]));
        }
    }
    pq->count = 0;
}

// Phase 37 slice B: element-kind flags. The compiler emits exactly
// one of these right after a container alloc, only when the static
// element type is an owned type (class/interface, array, or map).
// The alloc signatures are unchanged, so the ~20 runtime-internal
// callers and the C++ tests are untouched — and an unmarked container
// simply never releases elements (today's leak, never a wrong free).
// A setter on a null container is a no-op.
void sp_rt_array_mark_ref_elements(void* arr) {
    if (arr) static_cast<SPArrayHeader*>(arr)->elements_are_refs = true;
}

void sp_rt_list_mark_ref_elements(void* list) {
    if (list) static_cast<SPListImpl*>(list)->elements_are_refs = true;
}

void sp_rt_map_mark_ref_elements(void* map) {
    if (map) static_cast<SPMapImpl*>(map)->elements_are_refs = true;
}

void sp_rt_set_mark_ref_elements(void* set) {
    if (set) static_cast<SPSetImpl*>(set)->map.ref_elements_are_keys = true;
}

void sp_rt_queue_mark_ref_elements(void* queue) {
    if (queue) static_cast<SPQueueImpl*>(queue)->elements_are_refs = true;
}

void sp_rt_pq_mark_ref_elements(void* pq) {
    if (pq) static_cast<SPPQImpl*>(pq)->elements_are_refs = true;
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

// Phase 6 slice B: captured program arguments (deep-copied, owned by the
// runtime for the process lifetime; argv[0] — the program name — is kept
// so callers can inspect it, but sp_rt_os_args() returns argv[1..]).
static int g_sp_argc = 0;
static char** g_sp_argv = nullptr;

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
    // Phase 6 slice B: the non-Windows branch used to return an empty
    // array unconditionally (argv was never captured — the generated
    // `main` took no parameters). The generated `main` now forwards
    // argc/argv via sp_rt_set_args() before any user code runs.
    int count = g_sp_argc > 1 ? g_sp_argc - 1 : 0; // argv[0] is the program name
    void* arr = sp_rt_array_alloc(static_cast<int64_t>(count), sizeof(int64_t));
    for (int i = 0; i < count; ++i) {
        const char* arg_str = (g_sp_argv && g_sp_argv[i + 1]) ? g_sp_argv[i + 1] : "";
        size_t len = std::strlen(arg_str);
        char* copy = static_cast<char*>(malloc(len + 1));
        std::memcpy(copy, arg_str, len + 1);
        sp_rt_array_set_i64(arr, static_cast<int64_t>(i), reinterpret_cast<int64_t>(copy));
    }
    return arr;
#endif
}

// Phase 6 slice B: the generated `main` forwards argc/argv here before
// any user code runs (first call wins; main runs once).
void sp_rt_set_args(int argc, char** argv) {
    if (g_sp_argv) return; // first call wins (main runs once)
    g_sp_argc = argc > 0 ? argc : 0;
    if (argc <= 0 || !argv) {
        g_sp_argv = nullptr;
        return;
    }
    g_sp_argv = static_cast<char**>(std::calloc(static_cast<size_t>(argc), sizeof(char*)));
    for (int i = 0; i < argc; ++i) {
        const char* a = argv[i] ? argv[i] : "";
        size_t len = std::strlen(a);
        char* copy = static_cast<char*>(malloc(len + 1));
        std::memcpy(copy, a, len + 1);
        g_sp_argv[i] = copy;
    }
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

