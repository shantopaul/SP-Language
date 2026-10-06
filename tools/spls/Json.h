#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cctype>

namespace sp::lsp {

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::map<std::string, JsonValue> objVal;

    JsonValue() : type(JsonType::Null) {}
    JsonValue(std::nullptr_t) : type(JsonType::Null) {}
    JsonValue(bool b) : type(JsonType::Bool), boolVal(b) {}
    JsonValue(int n) : type(JsonType::Number), numVal(static_cast<double>(n)) {}
    JsonValue(int64_t n) : type(JsonType::Number), numVal(static_cast<double>(n)) {}
    JsonValue(uint32_t n) : type(JsonType::Number), numVal(static_cast<double>(n)) {}
    JsonValue(uint64_t n) : type(JsonType::Number), numVal(static_cast<double>(n)) {}
    JsonValue(double n) : type(JsonType::Number), numVal(n) {}
    JsonValue(const char* s) : type(JsonType::String), strVal(s ? s : "") {}
    JsonValue(std::string s) : type(JsonType::String), strVal(std::move(s)) {}

    static JsonValue array() {
        JsonValue v;
        v.type = JsonType::Array;
        return v;
    }

    static JsonValue object() {
        JsonValue v;
        v.type = JsonType::Object;
        return v;
    }

    void push_back(JsonValue v) {
        if (type != JsonType::Array) {
            type = JsonType::Array;
            arrVal.clear();
        }
        arrVal.push_back(std::move(v));
    }

    JsonValue& operator[](const std::string& key) {
        if (type != JsonType::Object) {
            type = JsonType::Object;
            objVal.clear();
        }
        return objVal[key];
    }

    const JsonValue& operator[](const std::string& key) const {
        static const JsonValue nullVal;
        if (type != JsonType::Object) return nullVal;
        auto it = objVal.find(key);
        return it != objVal.end() ? it->second : nullVal;
    }

    JsonValue& operator[](size_t idx) {
        if (type != JsonType::Array) {
            type = JsonType::Array;
            arrVal.clear();
        }
        if (idx >= arrVal.size()) arrVal.resize(idx + 1);
        return arrVal[idx];
    }

    const JsonValue& operator[](size_t idx) const {
        static const JsonValue nullVal;
        if (type != JsonType::Array || idx >= arrVal.size()) return nullVal;
        return arrVal[idx];
    }

    bool hasKey(const std::string& key) const {
        return type == JsonType::Object && objVal.find(key) != objVal.end();
    }

    [[nodiscard]] bool isNull() const noexcept { return type == JsonType::Null; }
    [[nodiscard]] bool isBool() const noexcept { return type == JsonType::Bool; }
    [[nodiscard]] bool isNumber() const noexcept { return type == JsonType::Number; }
    [[nodiscard]] bool isString() const noexcept { return type == JsonType::String; }
    [[nodiscard]] bool isArray() const noexcept { return type == JsonType::Array; }
    [[nodiscard]] bool isObject() const noexcept { return type == JsonType::Object; }

    [[nodiscard]] std::string asString(const std::string& def = "") const {
        return isString() ? strVal : def;
    }
    [[nodiscard]] int64_t asInt(int64_t def = 0) const {
        return isNumber() ? static_cast<int64_t>(numVal) : def;
    }
    [[nodiscard]] double asDouble(double def = 0.0) const {
        return isNumber() ? numVal : def;
    }
    [[nodiscard]] bool asBool(bool def = false) const {
        return isBool() ? boolVal : def;
    }

    std::string serialize() const {
        std::ostringstream ss;
        switch (type) {
            case JsonType::Null: ss << "null"; break;
            case JsonType::Bool: ss << (boolVal ? "true" : "false"); break;
            case JsonType::Number: {
                if (numVal == static_cast<int64_t>(numVal)) {
                    ss << static_cast<int64_t>(numVal);
                } else {
                    ss << numVal;
                }
                break;
            }
            case JsonType::String: {
                ss << "\"";
                for (char c : strVal) {
                    switch (c) {
                        case '"': ss << "\\\""; break;
                        case '\\': ss << "\\\\"; break;
                        case '\b': ss << "\\b"; break;
                        case '\f': ss << "\\f"; break;
                        case '\n': ss << "\\n"; break;
                        case '\r': ss << "\\r"; break;
                        case '\t': ss << "\\t"; break;
                        default:
                            if (static_cast<unsigned char>(c) < 0x20) {
                                ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec;
                            } else {
                                ss << c;
                            }
                            break;
                    }
                }
                ss << "\"";
                break;
            }
            case JsonType::Array: {
                ss << "[";
                for (size_t i = 0; i < arrVal.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << arrVal[i].serialize();
                }
                ss << "]";
                break;
            }
            case JsonType::Object: {
                ss << "{";
                bool first = true;
                for (const auto& [k, v] : objVal) {
                    if (!first) ss << ",";
                    first = false;
                    ss << "\"" << k << "\":" << v.serialize();
                }
                ss << "}";
                break;
            }
        }
        return ss.str();
    }

    static JsonValue parse(const std::string& src) {
        size_t idx = 0;
        return parseValue(src, idx);
    }

private:
    static void skipWhitespace(const std::string& src, size_t& idx) {
        while (idx < src.size() && (src[idx] == ' ' || src[idx] == '\t' || src[idx] == '\n' || src[idx] == '\r')) {
            idx++;
        }
    }

    static std::string parseString(const std::string& src, size_t& idx) {
        std::string s;
        idx++; // skip opening '"'
        while (idx < src.size()) {
            char c = src[idx++];
            if (c == '"') {
                return s;
            }
            if (c == '\\' && idx < src.size()) {
                char esc = src[idx++];
                switch (esc) {
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    case '/': s += '/'; break;
                    case 'b': s += '\b'; break;
                    case 'f': s += '\f'; break;
                    case 'n': s += '\n'; break;
                    case 'r': s += '\r'; break;
                    case 't': s += '\t'; break;
                    case 'u': {
                        if (idx + 4 <= src.size()) {
                            // hex 4 chars (skip full unicode handling, placeholder)
                            idx += 4;
                            s += '?';
                        }
                        break;
                    }
                    default: s += esc; break;
                }
            } else {
                s += c;
            }
        }
        return s;
    }

    static JsonValue parseValue(const std::string& src, size_t& idx) {
        skipWhitespace(src, idx);
        if (idx >= src.size()) return JsonValue{};

        char c = src[idx];
        if (c == '"') {
            return JsonValue(parseString(src, idx));
        }
        if (c == '{') {
            idx++; // skip '{'
            JsonValue obj = JsonValue::object();
            while (true) {
                skipWhitespace(src, idx);
                if (idx >= src.size() || src[idx] == '}') {
                    if (idx < src.size()) idx++;
                    break;
                }
                if (src[idx] == ',') {
                    idx++;
                    continue;
                }
                if (src[idx] == '"') {
                    std::string key = parseString(src, idx);
                    skipWhitespace(src, idx);
                    if (idx < src.size() && src[idx] == ':') idx++;
                    JsonValue val = parseValue(src, idx);
                    obj[key] = std::move(val);
                } else {
                    idx++;
                }
            }
            return obj;
        }
        if (c == '[') {
            idx++; // skip '['
            JsonValue arr = JsonValue::array();
            while (true) {
                skipWhitespace(src, idx);
                if (idx >= src.size() || src[idx] == ']') {
                    if (idx < src.size()) idx++;
                    break;
                }
                if (src[idx] == ',') {
                    idx++;
                    continue;
                }
                arr.push_back(parseValue(src, idx));
            }
            return arr;
        }
        if (src.compare(idx, 4, "true") == 0) {
            idx += 4;
            return JsonValue(true);
        }
        if (src.compare(idx, 5, "false") == 0) {
            idx += 5;
            return JsonValue(false);
        }
        if (src.compare(idx, 4, "null") == 0) {
            idx += 4;
            return JsonValue{};
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            size_t start = idx;
            if (src[idx] == '-') idx++;
            while (idx < src.size() && (std::isdigit(static_cast<unsigned char>(src[idx])) || src[idx] == '.' || src[idx] == 'e' || src[idx] == 'E' || src[idx] == '+')) {
                idx++;
            }
            double val = std::stod(src.substr(start, idx - start));
            return JsonValue(val);
        }

        idx++;
        return JsonValue{};
    }
};

} // namespace sp::lsp
