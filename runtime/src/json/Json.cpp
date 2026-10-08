#include "sp/runtime/Json.h"
#include "sp/runtime/Runtime.h"
#include <sstream>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <charconv>
#include <cstring>
#include <array>

namespace sp::rt::json {

// ----------------------------------------------------------------------------
// JsonValue Methods
// ----------------------------------------------------------------------------

JsonValuePtr JsonValue::makeNull() {
    static const auto s_null = std::make_shared<JsonValue>(JsonType::Null);
    return s_null;
}

JsonValuePtr JsonValue::makeBool(bool val) {
    static const auto s_true = std::make_shared<JsonValue>(true);
    static const auto s_false = std::make_shared<JsonValue>(false);
    return val ? s_true : s_false;
}

JsonValuePtr JsonValue::makeInt(int64_t val) {
    if (val >= 0 && val < 256) {
        static const auto s_cache = []() {
            std::array<JsonValuePtr, 256> cache;
            for (size_t i = 0; i < 256; ++i) {
                cache[i] = std::make_shared<JsonValue>(static_cast<int64_t>(i));
            }
            return cache;
        }();
        return s_cache[static_cast<size_t>(val)];
    }
    return std::make_shared<JsonValue>(val);
}

void JsonValue::appendMember(std::string key, JsonValuePtr value) {
    if (m_type != JsonType::Object) {
        m_type = JsonType::Object;
    }
    m_members.emplace_back(std::move(key), value ? value : makeNull());
}

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

void JsonValue::set(std::string_view key, JsonValuePtr value) {
    if (m_type != JsonType::Object) {
        m_type = JsonType::Object;
    }
    for (auto& member : m_members) {
        if (member.first == key) {
            member.second = value ? value : makeNull();
            return;
        }
    }
    m_members.emplace_back(std::string(key), value ? value : makeNull());
}

JsonValuePtr JsonValue::get(std::string_view key) const {
    if (m_type != JsonType::Object) return nullptr;
    for (const auto& member : m_members) {
        if (member.first == key) {
            return member.second;
        }
    }
    return nullptr;
}

bool JsonValue::has(std::string_view key) const {
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

void escapeJsonString(std::string_view input, std::ostringstream& ss) {
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
                if (s.find('.') != std::string::npos) {
                    while (s.size() > 1 && s.back() == '0') s.pop_back();
                    if (s.back() == '.') s.push_back('0');
                }
                ss << s;
            }
            break;
        }
        case JsonType::String:
            escapeJsonString(val.asStringView(), ss);
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
// Zero-Copy & SIMD-Accelerated Parser
// ----------------------------------------------------------------------------

namespace {

class FastJsonParser {
public:
    explicit FastJsonParser(std::string_view src) : m_src(src), m_pos(0) {}

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

    char peek() const noexcept {
        return m_pos < m_src.size() ? m_src[m_pos] : '\0';
    }

    char get() noexcept {
        return m_pos < m_src.size() ? m_src[m_pos++] : '\0';
    }

    // Fast whitespace scanner: instantaneous return on non-whitespace
    void skipWhitespace() noexcept {
        while (m_pos < m_src.size()) {
            unsigned char c = static_cast<unsigned char>(m_src[m_pos]);
            if (c > 0x20) return;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                m_pos++;
            } else {
                return;
            }
        }
    }

    JsonValuePtr parseValue() {
        skipWhitespace();
        if (m_pos >= m_src.size()) {
            throw std::runtime_error("Unexpected end of input");
        }
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
        if (m_pos + 4 <= m_src.size() && m_src.substr(m_pos, 4) == "null") {
            m_pos += 4;
            return JsonValue::makeNull();
        }
        throw std::runtime_error("Expected 'null'");
    }

    JsonValuePtr parseBool() {
        if (m_pos + 4 <= m_src.size() && m_src.substr(m_pos, 4) == "true") {
            m_pos += 4;
            return JsonValue::makeBool(true);
        }
        if (m_pos + 5 <= m_src.size() && m_src.substr(m_pos, 5) == "false") {
            m_pos += 5;
            return JsonValue::makeBool(false);
        }
        throw std::runtime_error("Expected boolean");
    }

    // Zero-copy string parser
    // Returns true if outOwned was populated (has escapes), false if outView was borrowed
    bool parseStringContent(std::string_view& outView, std::string& outOwned) {
        if (get() != '"') throw std::runtime_error("Expected '\"'");
        size_t start = m_pos;

        // Hardware-accelerated search for quote or backslash
        size_t nextSpecial = m_src.find_first_of("\"\\", m_pos);
        if (nextSpecial != std::string_view::npos && m_src[nextSpecial] == '"') {
            bool hasControl = false;
            for (size_t i = start; i < nextSpecial; ++i) {
                if (static_cast<unsigned char>(m_src[i]) < 0x20) {
                    hasControl = true;
                    break;
                }
            }
            if (!hasControl) {
                outView = m_src.substr(start, nextSpecial - start);
                m_pos = nextSpecial + 1; // consume closing '"'
                return false; // borrowed zero-copy view!
            }
        }

        bool hasEscape = false;

        // Fallback scan forward looking for '"' or '\\' or control chars
        while (m_pos < m_src.size()) {
            char c = m_src[m_pos];
            if (c == '"') {
                if (!hasEscape) {
                    outView = m_src.substr(start, m_pos - start);
                    m_pos++; // consume closing '"'
                    return false; // borrowed zero-copy view!
                }
                break;
            }
            if (c == '\\') {
                hasEscape = true;
                break;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                throw std::runtime_error("Unescaped control character in string (RFC 8259)");
            }
            m_pos++;
        }

        if (m_pos >= m_src.size()) {
            throw std::runtime_error("Unterminated string");
        }

        // Full unescaping pass into outOwned
        m_pos = start;
        outOwned.clear();
        outOwned.reserve(32);

        while (m_pos < m_src.size()) {
            char c = get();
            if (c == '"') {
                return true; // owned string
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                throw std::runtime_error("Unescaped control character in string (RFC 8259)");
            }
            if (c == '\\') {
                if (m_pos >= m_src.size()) throw std::runtime_error("Unterminated escape sequence");
                char esc = get();
                switch (esc) {
                    case '"':  outOwned.push_back('"'); break;
                    case '\\': outOwned.push_back('\\'); break;
                    case '/':  outOwned.push_back('/'); break;
                    case 'b':  outOwned.push_back('\b'); break;
                    case 'f':  outOwned.push_back('\f'); break;
                    case 'n':  outOwned.push_back('\n'); break;
                    case 'r':  outOwned.push_back('\r'); break;
                    case 't':  outOwned.push_back('\t'); break;
                    case 'u': {
                        if (m_pos + 4 > m_src.size()) throw std::runtime_error("Incomplete unicode escape");
                        uint32_t cp = 0;
                        for (int i = 0; i < 4; ++i) {
                            char hc = get();
                            cp <<= 4;
                            if (hc >= '0' && hc <= '9') cp |= (hc - '0');
                            else if (hc >= 'a' && hc <= 'f') cp |= (hc - 'a' + 10);
                            else if (hc >= 'A' && hc <= 'F') cp |= (hc - 'A' + 10);
                            else throw std::runtime_error("Invalid hex digit in \\uXXXX");
                        }
                        // Handle UTF-16 surrogate pairs
                        if (cp >= 0xD800 && cp <= 0xDBFF) {
                            if (m_pos + 6 <= m_src.size() && m_src[m_pos] == '\\' && m_src[m_pos + 1] == 'u') {
                                m_pos += 2;
                                uint32_t cp2 = 0;
                                for (int i = 0; i < 4; ++i) {
                                    char hc = get();
                                    cp2 <<= 4;
                                    if (hc >= '0' && hc <= '9') cp2 |= (hc - '0');
                                    else if (hc >= 'a' && hc <= 'f') cp2 |= (hc - 'a' + 10);
                                    else if (hc >= 'A' && hc <= 'F') cp2 |= (hc - 'A' + 10);
                                    else throw std::runtime_error("Invalid hex in low surrogate");
                                }
                                if (cp2 >= 0xDC00 && cp2 <= 0xDFFF) {
                                    cp = 0x10000 + (((cp - 0xD800) << 10) | (cp2 - 0xDC00));
                                }
                            }
                        }
                        // Encode as UTF-8
                        if (cp < 0x80) {
                            outOwned.push_back(static_cast<char>(cp));
                        } else if (cp < 0x800) {
                            outOwned.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                            outOwned.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                        } else if (cp < 0x10000) {
                            outOwned.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                            outOwned.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                            outOwned.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                        } else {
                            outOwned.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                            outOwned.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                            outOwned.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                            outOwned.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error(std::string("Invalid escape sequence: \\") + esc);
                }
            } else {
                outOwned.push_back(c);
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    JsonValuePtr parseString() {
        std::string_view view;
        std::string owned;
        if (parseStringContent(view, owned)) {
            return JsonValue::makeString(std::move(owned));
        } else {
            return JsonValue::makeStringView(view);
        }
    }

    // In-situ number parser: zero intermediate allocations
    JsonValuePtr parseNumber() {
        size_t start = m_pos;
        bool isNeg = false;
        if (peek() == '-') {
            isNeg = true;
            get();
        }
        if (m_pos >= m_src.size()) throw std::runtime_error("Invalid number format");
        char firstDigit = peek();
        if (firstDigit < '0' || firstDigit > '9') throw std::runtime_error("Invalid number format");

        // RFC 8259: Leading zeroes are not allowed in numbers
        if (firstDigit == '0' && m_pos + 1 < m_src.size()) {
            char nextC = m_src[m_pos + 1];
            if (nextC >= '0' && nextC <= '9') {
                throw std::runtime_error("Numbers cannot have leading zeroes (RFC 8259)");
            }
        }

        bool isFloat = false;
        int64_t intVal = 0;
        bool intOverflow = false;

        while (m_pos < m_src.size() && m_src[m_pos] >= '0' && m_src[m_pos] <= '9') {
            int digit = m_src[m_pos] - '0';
            if (intVal > (INT64_MAX - digit) / 10) {
                intOverflow = true;
            } else {
                intVal = intVal * 10 + digit;
            }
            m_pos++;
        }

        if (peek() == '.') {
            isFloat = true;
            get();
            if (m_pos >= m_src.size() || m_src[m_pos] < '0' || m_src[m_pos] > '9') {
                throw std::runtime_error("Fraction must have at least one digit");
            }
            while (m_pos < m_src.size() && m_src[m_pos] >= '0' && m_src[m_pos] <= '9') m_pos++;
        }

        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            get();
            if (peek() == '+' || peek() == '-') get();
            if (m_pos >= m_src.size() || m_src[m_pos] < '0' || m_src[m_pos] > '9') {
                throw std::runtime_error("Exponent must have at least one digit");
            }
            while (m_pos < m_src.size() && m_src[m_pos] >= '0' && m_src[m_pos] <= '9') m_pos++;
        }

        if (isFloat || intOverflow) {
            double dVal = 0.0;
            auto [ptr, ec] = std::from_chars(m_src.data() + start, m_src.data() + m_pos, dVal);
            if (ec != std::errc{}) {
                std::string s(m_src.substr(start, m_pos - start));
                dVal = std::stod(s);
            }
            return JsonValue::makeDouble(dVal);
        }

        if (isNeg) intVal = -intVal;
        return JsonValue::makeInt(intVal);
    }

    JsonValuePtr parseArray() {
        if (get() != '[') throw std::runtime_error("Expected '['");
        auto arr = JsonValue::makeArray();
        arr->reserveArray(16);
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
            // Trailing comma check
            if (peek() == ']') {
                throw std::runtime_error("Trailing comma not allowed in array (RFC 8259)");
            }
        }
        return arr;
    }

    JsonValuePtr parseObject() {
        if (get() != '{') throw std::runtime_error("Expected '{'");
        auto obj = JsonValue::makeObject();
        obj->reserveMembers(8);
        skipWhitespace();
        if (peek() == '}') {
            get();
            return obj;
        }

        while (true) {
            skipWhitespace();
            if (peek() != '"') throw std::runtime_error("Expected string key in object (unquoted keys rejected)");
            std::string_view keyView;
            std::string keyOwned;
            bool wasOwned = parseStringContent(keyView, keyOwned);

            skipWhitespace();
            if (get() != ':') throw std::runtime_error("Expected ':' after key in object");
            skipWhitespace();
            auto val = parseValue();
            if (wasOwned) {
                obj->appendMember(std::move(keyOwned), std::move(val));
            } else {
                obj->appendMember(std::string(keyView), std::move(val));
            }

            skipWhitespace();
            char c = get();
            if (c == '}') break;
            if (c != ',') throw std::runtime_error("Expected ',' or '}' in object");
            skipWhitespace();
            // Trailing comma check
            if (peek() == '}') {
                throw std::runtime_error("Trailing comma not allowed in object (RFC 8259)");
            }
        }
        return obj;
    }
};

} // anonymous namespace

JsonValuePtr parse(std::string_view jsonStr, std::string* outError) {
    FastJsonParser parser(jsonStr);
    return parser.parse(outError);
}

bool validate(std::string_view jsonStr, std::string* outError) {
    size_t pos = 0;
    auto skipWs = [&]() {
        while (pos < jsonStr.size()) {
            unsigned char c = static_cast<unsigned char>(jsonStr[pos]);
            if (c > 0x20) return;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') pos++;
            else return;
        }
    };

    try {
        skipWs();
        if (pos >= jsonStr.size()) {
            if (outError) *outError = "Empty input";
            return false;
        }

        auto validateVal = [&](auto& self) -> void {
            skipWs();
            if (pos >= jsonStr.size()) throw std::runtime_error("Unexpected end");
            char c = jsonStr[pos];
            if (c == 'n') {
                if (pos + 4 <= jsonStr.size() && jsonStr.substr(pos, 4) == "null") { pos += 4; return; }
                throw std::runtime_error("Invalid literal");
            }
            if (c == 't') {
                if (pos + 4 <= jsonStr.size() && jsonStr.substr(pos, 4) == "true") { pos += 4; return; }
                throw std::runtime_error("Invalid literal");
            }
            if (c == 'f') {
                if (pos + 5 <= jsonStr.size() && jsonStr.substr(pos, 5) == "false") { pos += 5; return; }
                throw std::runtime_error("Invalid literal");
            }
            if (c == '"') {
                pos++;
                size_t nextQuote = jsonStr.find_first_of("\"\\", pos);
                while (nextQuote != std::string_view::npos) {
                    if (jsonStr[nextQuote] == '"') {
                        pos = nextQuote + 1;
                        return;
                    }
                    pos = nextQuote + 2;
                    if (pos > jsonStr.size()) throw std::runtime_error("Unterminated escape");
                    nextQuote = jsonStr.find_first_of("\"\\", pos);
                }
                throw std::runtime_error("Unterminated string");
            }
            if (c == '[') {
                pos++;
                skipWs();
                if (pos < jsonStr.size() && jsonStr[pos] == ']') { pos++; return; }
                while (true) {
                    self(self);
                    skipWs();
                    if (pos >= jsonStr.size()) throw std::runtime_error("Unterminated array");
                    if (jsonStr[pos] == ']') { pos++; break; }
                    if (jsonStr[pos] != ',') throw std::runtime_error("Expected ',' or ']'");
                    pos++;
                    skipWs();
                    if (pos < jsonStr.size() && jsonStr[pos] == ']') throw std::runtime_error("Trailing comma");
                }
                return;
            }
            if (c == '{') {
                pos++;
                skipWs();
                if (pos < jsonStr.size() && jsonStr[pos] == '}') { pos++; return; }
                while (true) {
                    skipWs();
                    if (pos >= jsonStr.size() || jsonStr[pos] != '"') throw std::runtime_error("Expected string key");
                    pos++;
                    size_t nextQuote = jsonStr.find_first_of("\"\\", pos);
                    while (nextQuote != std::string_view::npos) {
                        if (jsonStr[nextQuote] == '"') { pos = nextQuote + 1; break; }
                        pos = nextQuote + 2;
                        if (pos > jsonStr.size()) throw std::runtime_error("Unterminated escape");
                        nextQuote = jsonStr.find_first_of("\"\\", pos);
                    }
                    skipWs();
                    if (pos >= jsonStr.size() || jsonStr[pos] != ':') throw std::runtime_error("Expected ':'");
                    pos++;
                    self(self);
                    skipWs();
                    if (pos >= jsonStr.size()) throw std::runtime_error("Unterminated object");
                    if (jsonStr[pos] == '}') { pos++; break; }
                    if (jsonStr[pos] != ',') throw std::runtime_error("Expected ',' or '}'");
                    pos++;
                    skipWs();
                    if (pos < jsonStr.size() && jsonStr[pos] == '}') throw std::runtime_error("Trailing comma");
                }
                return;
            }
            if (c == '-' || (c >= '0' && c <= '9')) {
                if (c == '0' && pos + 1 < jsonStr.size() && jsonStr[pos + 1] >= '0' && jsonStr[pos + 1] <= '9') {
                    throw std::runtime_error("Leading zero");
                }
                pos++;
                while (pos < jsonStr.size()) {
                    char nc = jsonStr[pos];
                    if ((nc >= '0' && nc <= '9') || nc == '.' || nc == 'e' || nc == 'E' || nc == '+' || nc == '-') pos++;
                    else break;
                }
                return;
            }
            throw std::runtime_error("Invalid character");
        };

        validateVal(validateVal);
        skipWs();
        if (pos < jsonStr.size()) {
            if (outError) *outError = "Trailing characters";
            return false;
        }
        return true;
    } catch (const std::exception& ex) {
        if (outError) *outError = ex.what();
        return false;
    }
}

// ----------------------------------------------------------------------------
// Class Metadata Registry & Typed Serialization Implementation (SP 2.4)
// ----------------------------------------------------------------------------

const FieldMetadata* ClassMetadata::findFieldByJsonKey(std::string_view key) const {
    for (const auto& f : fields) {
        if (f.jsonKey == key) return &f;
    }
    return nullptr;
}

const FieldMetadata* ClassMetadata::findFieldByName(std::string_view name) const {
    for (const auto& f : fields) {
        if (f.fieldName == name) return &f;
    }
    return nullptr;
}

ClassMetadataRegistry& ClassMetadataRegistry::instance() {
    static ClassMetadataRegistry s_instance;
    return s_instance;
}

void ClassMetadataRegistry::registerClass(ClassMetadata meta) {
    m_classes[meta.className] = std::move(meta);
}

const ClassMetadata* ClassMetadataRegistry::findClass(std::string_view className) const {
    auto it = m_classes.find(std::string(className));
    if (it != m_classes.end()) return &it->second;
    return nullptr;
}

bool ClassMetadataRegistry::hasClass(std::string_view className) const {
    return m_classes.find(std::string(className)) != m_classes.end();
}

void ClassMetadataRegistry::clear() {
    m_classes.clear();
}

void* decodeTypedObject(const std::string& className, std::string_view jsonStr, std::string* outError) {
    const auto* meta = ClassMetadataRegistry::instance().findClass(className);
    if (!meta) {
        if (outError) *outError = "Class metadata not found for: " + className;
        return nullptr;
    }

    auto json = parse(jsonStr, outError);
    if (!json || !json->isObject()) {
        if (outError && outError->empty()) *outError = "Expected JSON object";
        return nullptr;
    }

    // Allocate object buffer
    size_t sz = meta->objectSize > 0 ? meta->objectSize : 128;
    char* obj = static_cast<char*>(sp_rt_alloc(sz));
    std::memset(obj, 0, sz);

    for (const auto& field : meta->fields) {
        if (field.isIgnored) continue;

        auto item = json->get(field.jsonKey);
        if (!item || item->isNull()) continue;

        char* fieldAddr = obj + field.byteOffset;
        switch (field.typeKind) {
            case FieldTypeKind::Int: {
                int32_t val = static_cast<int32_t>(item->asInt());
                std::memcpy(fieldAddr, &val, sizeof(int32_t));
                break;
            }
            case FieldTypeKind::Int64: {
                int64_t val = item->asInt();
                std::memcpy(fieldAddr, &val, sizeof(int64_t));
                break;
            }
            case FieldTypeKind::Double: {
                double val = item->asDouble();
                std::memcpy(fieldAddr, &val, sizeof(double));
                break;
            }
            case FieldTypeKind::Float: {
                float val = static_cast<float>(item->asDouble());
                std::memcpy(fieldAddr, &val, sizeof(float));
                break;
            }
            case FieldTypeKind::Bool: {
                bool val = item->asBool();
                std::memcpy(fieldAddr, &val, sizeof(bool));
                break;
            }
            case FieldTypeKind::String: {
                std::string_view sv = item->asStringView();
                char* sbuf = static_cast<char*>(sp_rt_alloc(sv.size() + 1));
                std::memcpy(sbuf, sv.data(), sv.size());
                sbuf[sv.size()] = '\0';
                const char* cstr = sbuf;
                std::memcpy(fieldAddr, &cstr, sizeof(const char*));
                break;
            }
            case FieldTypeKind::Object: {
                std::string nestedJson = item->stringify(0);
                void* nestedObj = decodeTypedObject(field.typeName, nestedJson, outError);
                std::memcpy(fieldAddr, &nestedObj, sizeof(void*));
                break;
            }
            default:
                break;
        }
    }

    return obj;
}

std::string encodeTypedObject(const std::string& className, const void* objPtr, int indent) {
    if (!objPtr) return "null";
    const auto* meta = ClassMetadataRegistry::instance().findClass(className);
    if (!meta) return "{}";

    const char* obj = static_cast<const char*>(objPtr);
    auto jsonObj = JsonValue::makeObject();

    for (const auto& field : meta->fields) {
        if (field.isIgnored) continue;

        const char* fieldAddr = obj + field.byteOffset;
        switch (field.typeKind) {
            case FieldTypeKind::Int: {
                int32_t val = 0;
                std::memcpy(&val, fieldAddr, sizeof(int32_t));
                jsonObj->set(field.jsonKey, JsonValue::makeInt(val));
                break;
            }
            case FieldTypeKind::Int64: {
                int64_t val = 0;
                std::memcpy(&val, fieldAddr, sizeof(int64_t));
                jsonObj->set(field.jsonKey, JsonValue::makeInt(val));
                break;
            }
            case FieldTypeKind::Double: {
                double val = 0.0;
                std::memcpy(&val, fieldAddr, sizeof(double));
                jsonObj->set(field.jsonKey, JsonValue::makeDouble(val));
                break;
            }
            case FieldTypeKind::Float: {
                float val = 0.0f;
                std::memcpy(&val, fieldAddr, sizeof(float));
                jsonObj->set(field.jsonKey, JsonValue::makeDouble(val));
                break;
            }
            case FieldTypeKind::Bool: {
                bool val = false;
                std::memcpy(&val, fieldAddr, sizeof(bool));
                jsonObj->set(field.jsonKey, JsonValue::makeBool(val));
                break;
            }
            case FieldTypeKind::String: {
                const char* cstr = nullptr;
                std::memcpy(&cstr, fieldAddr, sizeof(const char*));
                if (cstr) {
                    jsonObj->set(field.jsonKey, JsonValue::makeString(cstr));
                } else {
                    jsonObj->set(field.jsonKey, JsonValue::makeNull());
                }
                break;
            }
            case FieldTypeKind::Object: {
                const void* nestedObj = nullptr;
                std::memcpy(&nestedObj, fieldAddr, sizeof(const void*));
                if (nestedObj) {
                    std::string nestedStr = encodeTypedObject(field.typeName, nestedObj, 0);
                    auto nestedVal = parse(nestedStr);
                    jsonObj->set(field.jsonKey, nestedVal);
                } else {
                    jsonObj->set(field.jsonKey, JsonValue::makeNull());
                }
                break;
            }
            default:
                break;
        }
    }

    return jsonObj->stringify(indent);
}

// ----------------------------------------------------------------------------
// Problem Details (RFC 7807) Implementation
// ----------------------------------------------------------------------------

std::string ProblemDetails::toJsonString(int indent) const {
    auto root = JsonValue::makeObject();
    root->set("type", JsonValue::makeString(type));
    root->set("title", JsonValue::makeString(title));
    root->set("status", JsonValue::makeInt(status));
    root->set("detail", JsonValue::makeString(detail));
    if (!instance.empty()) {
        root->set("instance", JsonValue::makeString(instance));
    }
    auto errsArr = JsonValue::makeArray();
    for (const auto& err : errors) {
        auto errObj = JsonValue::makeObject();
        errObj->set("field", JsonValue::makeString(err.field));
        errObj->set("message", JsonValue::makeString(err.message));
        errsArr->add(errObj);
    }
    root->set("errors", errsArr);
    return root->stringify(indent);
}

} // namespace sp::rt::json
