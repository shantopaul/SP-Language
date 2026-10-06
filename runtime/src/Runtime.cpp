#include "sp/runtime/Runtime.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

void sp_rt_print_uint(uint64_t val) {
    std::cout << val;
}

void sp_rt_print_float(float val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.7g", val);
    std::cout << buf;
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

}

