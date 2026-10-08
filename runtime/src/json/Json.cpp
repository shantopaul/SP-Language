#include "sp/runtime/Json.h"
#include <sstream>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace sp::rt::json {

// ----------------------------------------------------------------------------
// JsonValue Methods
// ----------------------------------------------------------------------------

size_t JsonValue::size() const {
    if (m_type == JsonType::Array) return m_arrayItems.size();
    if (m_type == JsonType::Object) return m_members.size();
    return 0;
}

void JsonValue::add(JsonValuePtr item) {
    if (m_type != JsonType::Array) {
        m_type = JsonType::Array;
    }
    m_arrayItems.push_back(item ? item : makeNull());
}

JsonValuePtr JsonValue::get(size_t index) const {
    if (m_type == JsonType::Array && index < m_arrayItems.size()) {
        return m_arrayItems[index];
    }
    return nullptr;
}

void JsonValue::set(const std::string& key, JsonValuePtr value) {
    if (m_type != JsonType::Object) {
        m_type = JsonType::Object;
    }
    for (auto& member : m_members) {
        if (member.first == key) {
            member.second = value ? value : makeNull();
            return;
        }
    }
    m_members.emplace_back(key, value ? value : makeNull());
}

JsonValuePtr JsonValue::get(const std::string& key) const {
    if (m_type != JsonType::Object) return nullptr;
    for (const auto& member : m_members) {
        if (member.first == key) {
            return member.second;
        }
    }
    return nullptr;
}

bool JsonValue::has(const std::string& key) const {
    if (m_type != JsonType::Object) return false;
    for (const auto& member : m_members) {
        if (member.first == key) return true;
    }
    return false;
}

std::vector<std::string> JsonValue::getKeys() const {
    std::vector<std::string> keys;
    if (m_type == JsonType::Object) {
        keys.reserve(m_members.size());
        for (const auto& member : m_members) {
            keys.push_back(member.first);
        }
    }
    return keys;
}

namespace {

void escapeJsonString(const std::string& input, std::ostringstream& ss) {
    ss << '"';
    for (char c : input) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    ss << buf;
                } else {
                    ss << c;
                }
                break;
        }
    }
    ss << '"';
}

void stringifyInternal(const JsonValue& val, std::ostringstream& ss, int indent, int currentLevel) {
    std::string indentStr = (indent > 0) ? std::string(currentLevel * indent, ' ') : "";
    std::string nextIndentStr = (indent > 0) ? std::string((currentLevel + 1) * indent, ' ') : "";
    std::string newline = (indent > 0) ? "\n" : "";
    std::string space = (indent > 0) ? " " : "";

    switch (val.getType()) {
        case JsonType::Null:
            ss << "null";
            break;
        case JsonType::Bool:
            ss << (val.asBool() ? "true" : "false");
            break;
        case JsonType::Int:
            ss << val.asInt();
            break;
        case JsonType::Double: {
            double d = val.asDouble();
            if (std::isnan(d) || std::isinf(d)) {
                ss << "null";
            } else {
                std::string s = std::to_string(d);
                // Strip trailing zeros if decimal
                if (s.find('.') != std::string::npos) {
                    while (s.back() == '0') s.pop_back();
                    if (s.back() == '.') s.push_back('0');
                }
                ss << s;
            }
            break;
        }
        case JsonType::String:
            escapeJsonString(val.asString(), ss);
            break;
        case JsonType::Array: {
            const auto& items = val.getArrayItems();
            if (items.empty()) {
                ss << "[]";
                break;
            }
            ss << "[" << newline;
            for (size_t i = 0; i < items.size(); ++i) {
                ss << nextIndentStr;
                stringifyInternal(*items[i], ss, indent, currentLevel + 1);
                if (i + 1 < items.size()) {
                    ss << ",";
                }
                ss << newline;
            }
            ss << indentStr << "]";
            break;
        }
        case JsonType::Object: {
            const auto& members = val.getMembers();
            if (members.empty()) {
                ss << "{}";
                break;
            }
            ss << "{" << newline;
            for (size_t i = 0; i < members.size(); ++i) {
                ss << nextIndentStr;
                escapeJsonString(members[i].first, ss);
                ss << ":" << space;
                stringifyInternal(*members[i].second, ss, indent, currentLevel + 1);
                if (i + 1 < members.size()) {
                    ss << ",";
                }
                ss << newline;
            }
            ss << indentStr << "}";
            break;
        }
    }
}

} // anonymous namespace

std::string JsonValue::stringify(int indent) const {
    std::ostringstream ss;
    stringifyInternal(*this, ss, indent, 0);
    return ss.str();
}

// ----------------------------------------------------------------------------
// Parser Implementation
// ----------------------------------------------------------------------------

namespace {

class JsonParser {
public:
    explicit JsonParser(std::string_view src) : m_src(src), m_pos(0) {}

    JsonValuePtr parse(std::string* outError) {
        skipWhitespace();
        if (m_pos >= m_src.size()) {
            if (outError) *outError = "Empty input";
            return nullptr;
        }

        try {
            auto res = parseValue();
            skipWhitespace();
            if (m_pos < m_src.size()) {
                if (outError) *outError = "Trailing characters after JSON value";
                return nullptr;
            }
            return res;
        } catch (const std::exception& ex) {
            if (outError) *outError = ex.what();
            return nullptr;
        }
    }

private:
    std::string_view m_src;
    size_t m_pos{0};

    char peek() const {
        return m_pos < m_src.size() ? m_src[m_pos] : '\0';
    }

    char get() {
        return m_pos < m_src.size() ? m_src[m_pos++] : '\0';
    }

    void skipWhitespace() {
        while (m_pos < m_src.size()) {
            char c = m_src[m_pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                m_pos++;
            } else {
                break;
            }
        }
    }

    JsonValuePtr parseValue() {
        skipWhitespace();
        char c = peek();
        if (c == 'n') return parseNull();
        if (c == 't' || c == 'f') return parseBool();
        if (c == '"') return parseString();
        if (c == '[') return parseArray();
        if (c == '{') return parseObject();
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();

        throw std::runtime_error(std::string("Unexpected character: '") + c + "' at position " + std::to_string(m_pos));
    }

    JsonValuePtr parseNull() {
        if (m_src.substr(m_pos, 4) == "null") {
            m_pos += 4;
            return JsonValue::makeNull();
        }
        throw std::runtime_error("Expected 'null'");
    }

    JsonValuePtr parseBool() {
        if (m_src.substr(m_pos, 4) == "true") {
            m_pos += 4;
            return JsonValue::makeBool(true);
        }
        if (m_src.substr(m_pos, 5) == "false") {
            m_pos += 5;
            return JsonValue::makeBool(false);
        }
        throw std::runtime_error("Expected boolean");
    }

    std::string parseRawString() {
        if (get() != '"') throw std::runtime_error("Expected '\"'");
        std::string res;
        while (m_pos < m_src.size()) {
            char c = get();
            if (c == '"') {
                return res;
            }
            if (c == '\\') {
                char esc = get();
                switch (esc) {
                    case '"':  res.push_back('"'); break;
                    case '\\': res.push_back('\\'); break;
                    case '/':  res.push_back('/'); break;
                    case 'b':  res.push_back('\b'); break;
                    case 'f':  res.push_back('\f'); break;
                    case 'n':  res.push_back('\n'); break;
                    case 'r':  res.push_back('\r'); break;
                    case 't':  res.push_back('\t'); break;
                    case 'u': {
                        if (m_pos + 4 <= m_src.size()) {
                            std::string hexStr(m_src.substr(m_pos, 4));
                            m_pos += 4;
                            uint32_t cp = static_cast<uint32_t>(std::stoul(hexStr, nullptr, 16));
                            if (cp < 0x80) {
                                res.push_back(static_cast<char>(cp));
                            } else if (cp < 0x800) {
                                res.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                                res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                            } else {
                                res.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                                res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                                res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                            }
                        }
                        break;
                    }
                    default:
                        res.push_back(esc);
                        break;
                }
            } else {
                res.push_back(c);
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    JsonValuePtr parseString() {
        return JsonValue::makeString(parseRawString());
    }

    JsonValuePtr parseNumber() {
        size_t start = m_pos;
        if (peek() == '-') get();
        while (peek() >= '0' && peek() <= '9') get();
        bool isFloat = false;
        if (peek() == '.') {
            isFloat = true;
            get();
            while (peek() >= '0' && peek() <= '9') get();
        }
        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            get();
            if (peek() == '+' || peek() == '-') get();
            while (peek() >= '0' && peek() <= '9') get();
        }

        std::string numStr(m_src.substr(start, m_pos - start));
        if (isFloat) {
            double d = std::stod(numStr);
            return JsonValue::makeDouble(d);
        } else {
            int64_t i = std::stoll(numStr);
            return JsonValue::makeInt(i);
        }
    }

    JsonValuePtr parseArray() {
        if (get() != '[') throw std::runtime_error("Expected '['");
        auto arr = JsonValue::makeArray();
        skipWhitespace();
        if (peek() == ']') {
            get();
            return arr;
        }

        while (true) {
            arr->add(parseValue());
            skipWhitespace();
            char c = get();
            if (c == ']') break;
            if (c != ',') throw std::runtime_error("Expected ',' or ']' in array");
            skipWhitespace();
        }
        return arr;
    }

    JsonValuePtr parseObject() {
        if (get() != '{') throw std::runtime_error("Expected '{'");
        auto obj = JsonValue::makeObject();
        skipWhitespace();
        if (peek() == '}') {
            get();
            return obj;
        }

        while (true) {
            skipWhitespace();
            if (peek() != '"') throw std::runtime_error("Expected string key in object");
            std::string key = parseRawString();
            skipWhitespace();
            if (get() != ':') throw std::runtime_error("Expected ':' after key in object");
            skipWhitespace();
            auto val = parseValue();
            obj->set(key, val);
            skipWhitespace();
            char c = get();
            if (c == '}') break;
            if (c != ',') throw std::runtime_error("Expected ',' or '}' in object");
        }
        return obj;
    }
};

} // anonymous namespace

JsonValuePtr parse(std::string_view jsonStr, std::string* outError) {
    JsonParser parser(jsonStr);
    return parser.parse(outError);
}

} // namespace sp::rt::json
