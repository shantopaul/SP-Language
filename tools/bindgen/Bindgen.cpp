#include "bindgen/Bindgen.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iostream>

namespace sp::bindgen {

namespace {

bool isIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

std::vector<std::string> tokenizeC(const std::string& input, std::vector<CMacroConstant>& outMacros) {
    std::vector<std::string> tokens;
    size_t i = 0;
    size_t n = input.size();

    while (i < n) {
        // Skip whitespace
        if (isWhitespace(input[i])) {
            i++;
            continue;
        }

        // Line comment
        if (input[i] == '/' && i + 1 < n && input[i + 1] == '/') {
            i += 2;
            while (i < n && input[i] != '\n') i++;
            continue;
        }

        // Block comment
        if (input[i] == '/' && i + 1 < n && input[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(input[i] == '*' && input[i + 1] == '/')) i++;
            if (i + 1 < n) i += 2;
            continue;
        }

        // Preprocessor directive
        if (input[i] == '#') {
            i++; // skip #
            while (i < n && (input[i] == ' ' || input[i] == '\t')) i++;
            size_t dirStart = i;
            while (i < n && std::isalpha(static_cast<unsigned char>(input[i]))) i++;
            std::string directive = input.substr(dirStart, i - dirStart);

            if (directive == "define") {
                while (i < n && (input[i] == ' ' || input[i] == '\t')) i++;
                size_t nameStart = i;
                while (i < n && isIdentifierChar(input[i])) i++;
                std::string macroName = input.substr(nameStart, i - nameStart);

                // Only consider parameterless macro definitions
                if (i < n && input[i] != '(' && !macroName.empty()) {
                    while (i < n && (input[i] == ' ' || input[i] == '\t')) i++;
                    size_t valStart = i;
                    while (i < n && input[i] != '\n' && input[i] != '\r') {
                        // handle backslash line continuation
                        if (input[i] == '\\' && i + 1 < n && (input[i + 1] == '\n' || input[i + 1] == '\r')) {
                            i += 2;
                            continue;
                        }
                        i++;
                    }
                    std::string val = input.substr(valStart, i - valStart);
                    // Trim trailing whitespace
                    while (!val.empty() && isWhitespace(val.back())) val.pop_back();

                    if (!val.empty()) {
                        // Determine if it's a numeric constant or string literal
                        if (val.front() == '"' && val.back() == '"') {
                            outMacros.push_back({macroName, val, "string"});
                        } else {
                            bool isNumeric = true;
                            bool hasDot = false;
                            bool isHex = (val.size() > 2 && val[0] == '0' && (val[1] == 'x' || val[1] == 'X'));
                            size_t startIdx = 0;
                            if (val[0] == '-' || val[0] == '+') startIdx = 1;
                            if (isHex) startIdx = 2;

                            for (size_t k = startIdx; k < val.size(); ++k) {
                                char ch = val[k];
                                if (ch == '.') {
                                    hasDot = true;
                                } else if (ch == 'u' || ch == 'U' || ch == 'l' || ch == 'L' || ch == 'f' || ch == 'F') {
                                    // Suffix
                                    continue;
                                } else if (isHex) {
                                    if (!std::isxdigit(static_cast<unsigned char>(ch))) {
                                        isNumeric = false;
                                        break;
                                    }
                                } else {
                                    if (!std::isdigit(static_cast<unsigned char>(ch))) {
                                        isNumeric = false;
                                        break;
                                    }
                                }
                            }
                            if (isNumeric) {
                                std::string spType = hasDot ? "double" : "int32";
                                outMacros.push_back({macroName, val, spType});
                            }
                        }
                    }
                }
            }

            // Skip rest of preprocessor directive line (including continuation lines)
            while (i < n) {
                if (input[i] == '\\' && i + 1 < n) {
                    i += 2;
                    continue;
                }
                if (input[i] == '\n') {
                    i++;
                    break;
                }
                i++;
            }
            continue;
        }

        // String literal
        if (input[i] == '"') {
            size_t start = i++;
            while (i < n && input[i] != '"') {
                if (input[i] == '\\' && i + 1 < n) i += 2;
                else i++;
            }
            if (i < n) i++; // consume closing quote
            tokens.push_back(input.substr(start, i - start));
            continue;
        }

        // Character literal
        if (input[i] == '\'') {
            size_t start = i++;
            while (i < n && input[i] != '\'') {
                if (input[i] == '\\' && i + 1 < n) i += 2;
                else i++;
            }
            if (i < n) i++;
            tokens.push_back(input.substr(start, i - start));
            continue;
        }

        // Identifiers and numbers
        if (isIdentifierChar(input[i])) {
            size_t start = i;
            while (i < n && isIdentifierChar(input[i])) i++;
            tokens.push_back(input.substr(start, i - start));
            continue;
        }

        // Multi-character punctuation
        if (i + 2 < n && input.substr(i, 3) == "...") {
            tokens.push_back("...");
            i += 3;
            continue;
        }

        // Single punctuation characters
        tokens.push_back(std::string(1, input[i]));
        i++;
    }

    return tokens;
}

} // namespace

std::string CType::toSPType() const {
    std::string base = baseType;
    if (base == "void") {
        if (pointerDepth == 0) return "void";
        base = "void";
    } else if (base == "char") {
        base = isUnsigned ? "uint8" : "char";
    } else if (base == "signed char" || base == "int8_t") {
        base = "int8";
    } else if (base == "unsigned char" || base == "uint8_t" || base == "byte") {
        base = "uint8";
    } else if (base == "short" || base == "short int" || base == "int16_t") {
        base = isUnsigned ? "uint16" : "int16";
    } else if (base == "unsigned short" || base == "uint16_t") {
        base = "uint16";
    } else if (base == "int" || base == "signed int" || base == "int32_t") {
        base = isUnsigned ? "uint32" : "int32";
    } else if (base == "unsigned int" || base == "uint32_t" || base == "unsigned" || base == "uint") {
        base = "uint32";
    } else if (base == "long" || base == "long int" || base == "int64_t" || base == "long long") {
        base = isUnsigned ? "uint64" : "int64";
    } else if (base == "unsigned long" || base == "uint64_t" || base == "unsigned long long" || base == "size_t" || base == "uintptr_t") {
        base = "uint64";
    } else if (base == "ssize_t" || base == "intptr_t" || base == "ptrdiff_t") {
        base = "int64";
    } else if (base == "float") {
        base = "float";
    } else if (base == "double") {
        base = "double";
    } else if (base == "bool" || base == "_Bool") {
        base = "bool";
    }

    std::string result = base;
    for (int p = 0; p < pointerDepth; ++p) {
        result = "ptr<" + result + ">";
    }
    return result;
}

Bindgen::Bindgen(BindgenOptions options)
    : m_options(std::move(options)) {}

bool Bindgen::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_lastError = "Could not open C header file: " + filePath;
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return parseHeader(ss.str());
}

bool Bindgen::parseHeader(const std::string& headerContent) {
    m_structs.clear();
    m_functions.clear();
    m_enums.clear();
    m_macros.clear();
    m_knownStructNames.clear();
    m_knownEnumNames.clear();
    m_knownFunctionNames.clear();
    m_typedefAliases.clear();

    std::vector<CMacroConstant> extractedMacros;
    std::vector<std::string> tokens = tokenizeC(headerContent, extractedMacros);
    m_macros = std::move(extractedMacros);

    parseTokens(tokens);
    return true;
}

void Bindgen::skipAttributes(size_t& idx, const std::vector<std::string>& tokens) {
    while (idx < tokens.size()) {
        const std::string& tok = tokens[idx];
        if (tok == "__attribute__" || tok == "__declspec") {
            idx++;
            if (idx < tokens.size() && tokens[idx] == "(") {
                int depth = 1;
                idx++;
                while (idx < tokens.size() && depth > 0) {
                    if (tokens[idx] == "(") depth++;
                    else if (tokens[idx] == ")") depth--;
                    idx++;
                }
            }
        } else if (tok == "__cdecl" || tok == "__stdcall" || tok == "__fastcall" ||
                   tok == "__thiscall" || tok == "WINAPI" || tok == "APIENTRY" ||
                   tok == "GLAPI" || tok == "SP_API" || tok == "inline" ||
                   tok == "__inline" || tok == "__inline__" || tok == "__forceinline" ||
                   tok == "static" || tok == "extern") {
            idx++;
        } else {
            break;
        }
    }
}

CType Bindgen::parseCType(size_t& idx, const std::vector<std::string>& tokens) {
    CType type;
    skipAttributes(idx, tokens);

    bool hasUnsigned = false;
    bool hasSigned = false;
    bool hasConst = false;
    bool isStruct = false;

    while (idx < tokens.size()) {
        const std::string& tok = tokens[idx];
        if (tok == "const") {
            hasConst = true;
            idx++;
        } else if (tok == "volatile") {
            idx++;
        } else if (tok == "unsigned") {
            hasUnsigned = true;
            idx++;
        } else if (tok == "signed") {
            hasSigned = true;
            idx++;
        } else if (tok == "struct") {
            isStruct = true;
            idx++;
            if (idx < tokens.size() && isIdentifierChar(tokens[idx][0])) {
                type.baseType = tokens[idx];
                idx++;
            }
            break;
        } else if (tok == "enum") {
            idx++;
            if (idx < tokens.size() && isIdentifierChar(tokens[idx][0])) {
                type.baseType = tokens[idx];
                idx++;
            }
            break;
        } else {
            break;
        }
    }

    if (type.baseType.empty() && idx < tokens.size()) {
        std::string base = tokens[idx];
        idx++;
        // Check compound type words like "long int" or "long long"
        if (base == "long" && idx < tokens.size() && (tokens[idx] == "int" || tokens[idx] == "long")) {
            base += " " + tokens[idx];
            idx++;
        } else if (base == "short" && idx < tokens.size() && tokens[idx] == "int") {
            base += " " + tokens[idx];
            idx++;
        }
        type.baseType = base;
    } else if (type.baseType.empty() && (hasUnsigned || hasSigned)) {
        type.baseType = "int";
    }

    type.isConst = hasConst;
    type.isUnsigned = hasUnsigned;
    type.isStruct = isStruct;

    // Check pointer qualifiers
    while (idx < tokens.size()) {
        if (tokens[idx] == "*") {
            type.pointerDepth++;
            idx++;
        } else if (tokens[idx] == "const" || tokens[idx] == "volatile") {
            idx++;
        } else {
            break;
        }
    }

    return type;
}

void Bindgen::parseTokens(const std::vector<std::string>& tokens) {
    size_t i = 0;
    size_t n = tokens.size();

    while (i < n) {
        skipAttributes(i, tokens);
        if (i >= n) break;

        const std::string& tok = tokens[i];

        // 1. typedef
        if (tok == "typedef") {
            i++;
            skipAttributes(i, tokens);

            if (i < n && tokens[i] == "struct") {
                i++; // consume struct
                std::string tag;
                if (i < n && tokens[i] != "{" && tokens[i] != ";") {
                    tag = tokens[i++];
                }

                if (i < n && tokens[i] == "{") {
                    // Definition: typedef struct [tag] { ... } Alias;
                    i++; // consume {
                    CStruct st;
                    st.name = tag;

                    while (i < n && tokens[i] != "}") {
                        CType fieldType = parseCType(i, tokens);
                        if (i < n && isIdentifierChar(tokens[i][0])) {
                            std::string fieldName = tokens[i++];
                            int arrSize = 0;
                            if (i < n && tokens[i] == "[") {
                                i++; // [
                                if (i < n && std::isdigit(static_cast<unsigned char>(tokens[i][0]))) {
                                    arrSize = std::stoi(tokens[i++]);
                                }
                                if (i < n && tokens[i] == "]") i++;
                                fieldType.pointerDepth++; // treat array as pointer in SP struct
                            }
                            st.fields.push_back({fieldName, fieldType, arrSize});
                        }
                        if (i < n && tokens[i] == ";") i++;
                    }
                    if (i < n && tokens[i] == "}") i++;

                    // Alias name
                    skipAttributes(i, tokens);
                    if (i < n && tokens[i] != ";") {
                        st.name = tokens[i++];
                    }
                    if (i < n && tokens[i] == ";") i++;

                    if (!st.name.empty() && !m_knownStructNames.count(st.name)) {
                        m_knownStructNames.insert(st.name);
                        m_structs.push_back(st);
                    }
                    continue;
                } else if (i < n && tokens[i] != ";") {
                    // Opaque forward typedef: typedef struct Tag Alias;
                    std::string alias = tokens[i++];
                    if (i < n && tokens[i] == ";") i++;

                    CStruct st;
                    st.name = alias.empty() ? tag : alias;
                    st.isOpaque = true;
                    if (!st.name.empty() && !m_knownStructNames.count(st.name)) {
                        m_knownStructNames.insert(st.name);
                        m_structs.push_back(st);
                    }
                    continue;
                }
            } else if (i < n && tokens[i] == "enum") {
                i++; // consume enum
                std::string tag;
                if (i < n && tokens[i] != "{" && tokens[i] != ";") {
                    tag = tokens[i++];
                }
                if (i < n && tokens[i] == "{") {
                    i++; // {
                    CEnum en;
                    en.name = tag;
                    int64_t currentVal = 0;

                    while (i < n && tokens[i] != "}") {
                        if (isIdentifierChar(tokens[i][0])) {
                            std::string varName = tokens[i++];
                            std::optional<int64_t> valOpt;
                            if (i < n && tokens[i] == "=") {
                                i++; // =
                                if (i < n) {
                                    try {
                                        currentVal = std::stoll(tokens[i]);
                                        valOpt = currentVal;
                                    } catch (...) {}
                                    i++;
                                }
                            } else {
                                valOpt = currentVal;
                            }
                            en.variants.push_back({varName, valOpt});
                            currentVal++;
                        }
                        if (i < n && tokens[i] == ",") i++;
                    }
                    if (i < n && tokens[i] == "}") i++;

                    skipAttributes(i, tokens);
                    if (i < n && tokens[i] != ";") {
                        en.name = tokens[i++];
                    }
                    if (i < n && tokens[i] == ";") i++;

                    if (!en.name.empty() && !m_knownEnumNames.count(en.name)) {
                        m_knownEnumNames.insert(en.name);
                        m_enums.push_back(en);
                    }
                    continue;
                }
            } else {
                // Primitive typedef: typedef Type Alias;
                CType underlying = parseCType(i, tokens);
                if (i < n && isIdentifierChar(tokens[i][0])) {
                    std::string alias = tokens[i++];
                    m_typedefAliases[alias] = underlying;
                }
                while (i < n && tokens[i] != ";") i++;
                if (i < n && tokens[i] == ";") i++;
                continue;
            }
        }

        // 2. struct Name { ... }; or struct Name;
        if (tok == "struct") {
            i++;
            if (i < n && isIdentifierChar(tokens[i][0])) {
                std::string stName = tokens[i++];
                if (i < n && tokens[i] == "{") {
                    i++; // {
                    CStruct st;
                    st.name = stName;
                    while (i < n && tokens[i] != "}") {
                        CType fieldType = parseCType(i, tokens);
                        if (i < n && isIdentifierChar(tokens[i][0])) {
                            std::string fieldName = tokens[i++];
                            int arrSize = 0;
                            if (i < n && tokens[i] == "[") {
                                i++;
                                if (i < n && std::isdigit(static_cast<unsigned char>(tokens[i][0]))) {
                                    arrSize = std::stoi(tokens[i++]);
                                }
                                if (i < n && tokens[i] == "]") i++;
                                fieldType.pointerDepth++;
                            }
                            st.fields.push_back({fieldName, fieldType, arrSize});
                        }
                        if (i < n && tokens[i] == ";") i++;
                    }
                    if (i < n && tokens[i] == "}") i++;
                    if (i < n && tokens[i] == ";") i++;

                    if (!st.name.empty() && !m_knownStructNames.count(st.name)) {
                        m_knownStructNames.insert(st.name);
                        m_structs.push_back(st);
                    }
                    continue;
                } else if (i < n && tokens[i] == ";") {
                    i++; // ;
                    if (!stName.empty() && !m_knownStructNames.count(stName)) {
                        m_knownStructNames.insert(stName);
                        m_structs.push_back({stName, true, {}});
                    }
                    continue;
                }
            }
        }

        // 3. enum Name { ... };
        if (tok == "enum") {
            i++;
            if (i < n && isIdentifierChar(tokens[i][0])) {
                std::string enName = tokens[i++];
                if (i < n && tokens[i] == "{") {
                    i++;
                    CEnum en;
                    en.name = enName;
                    int64_t currentVal = 0;

                    while (i < n && tokens[i] != "}") {
                        if (isIdentifierChar(tokens[i][0])) {
                            std::string varName = tokens[i++];
                            std::optional<int64_t> valOpt;
                            if (i < n && tokens[i] == "=") {
                                i++;
                                if (i < n) {
                                    try {
                                        currentVal = std::stoll(tokens[i]);
                                        valOpt = currentVal;
                                    } catch (...) {}
                                    i++;
                                }
                            } else {
                                valOpt = currentVal;
                            }
                            en.variants.push_back({varName, valOpt});
                            currentVal++;
                        }
                        if (i < n && tokens[i] == ",") i++;
                    }
                    if (i < n && tokens[i] == "}") i++;
                    if (i < n && tokens[i] == ";") i++;

                    if (!en.name.empty() && !m_knownEnumNames.count(en.name)) {
                        m_knownEnumNames.insert(en.name);
                        m_enums.push_back(en);
                    }
                    continue;
                }
            }
        }

        // 4. Function declarations: ReturnType FuncName(Params);
        size_t checkpoint = i;
        CType retType = parseCType(i, tokens);
        skipAttributes(i, tokens);

        if (i < n && isIdentifierChar(tokens[i][0])) {
            std::string funcName = tokens[i++];
            if (i < n && tokens[i] == "(") {
                i++; // consume (
                CFunction fn;
                fn.name = funcName;
                fn.returnType = retType;

                int paramIdx = 0;
                while (i < n && tokens[i] != ")") {
                    if (tokens[i] == "...") {
                        fn.isVariadic = true;
                        i++;
                        break;
                    }
                    CType paramType = parseCType(i, tokens);
                    if (paramType.baseType == "void" && paramType.pointerDepth == 0) {
                        break; // (void) parameter list
                    }
                    std::string paramName;
                    if (i < n && isIdentifierChar(tokens[i][0]) && tokens[i] != "," && tokens[i] != ")") {
                        paramName = tokens[i++];
                    } else {
                        paramName = "arg" + std::to_string(paramIdx);
                    }
                    fn.params.push_back({paramName, paramType});
                    paramIdx++;

                    if (i < n && tokens[i] == ",") i++;
                }
                if (i < n && tokens[i] == ")") i++;
                skipAttributes(i, tokens);

                if (i < n && tokens[i] == ";") {
                    i++; // consume ;
                    if (!fn.name.empty() && !m_knownFunctionNames.count(fn.name)) {
                        m_knownFunctionNames.insert(fn.name);
                        m_functions.push_back(fn);
                    }
                    continue;
                }
            }
        }

        // Advance to next semicolon if unhandled statement
        i = checkpoint + 1;
        while (i < n && tokens[i] != ";") {
            if (tokens[i] == "}") { i++; break; }
            i++;
        }
        if (i < n && tokens[i] == ";") i++;
    }
}

std::string Bindgen::generateSP() const {
    std::ostringstream ss;

    ss << "// Generated by spc bindgen\n";
    ss << "// Direct C-ABI FFI bindings for SP Programming Language\n\n";

    if (!m_options.moduleName.empty()) {
        ss << "module " << m_options.moduleName << ";\n\n";
    }

    // Emit Preprocessor Constants
    if (!m_macros.empty()) {
        ss << "// === Constants ===\n";
        for (const auto& m : m_macros) {
            ss << "const " << m.name << ": " << m.spType << " = " << m.value << ";\n";
        }
        ss << "\n";
    }

    // Emit Enums
    if (!m_enums.empty()) {
        ss << "// === Enums ===\n";
        for (const auto& en : m_enums) {
            ss << "enum " << en.name << " {\n";
            for (const auto& var : en.variants) {
                ss << "    " << var.name;
                if (var.value.has_value()) {
                    ss << " = " << var.value.value();
                }
                ss << ",\n";
            }
            ss << "}\n\n";
        }
    }

    // Emit Structs
    if (!m_structs.empty()) {
        ss << "// === Structs ===\n";
        for (const auto& st : m_structs) {
            if (m_options.emitReprC) {
                ss << "#[repr(C)]\n";
            }
            ss << "struct " << st.name << " {\n";
            for (const auto& field : st.fields) {
                ss << "    " << field.name << ": " << field.type.toSPType() << ";\n";
            }
            ss << "}\n\n";
        }
    }

    // Emit Extern C Functions
    if (!m_functions.empty() && m_options.emitExternC) {
        ss << "// === Extern C Declarations ===\n";
        ss << "extern \"C\" {\n";
        for (const auto& fn : m_functions) {
            ss << "    fn " << fn.name << "(";
            for (size_t p = 0; p < fn.params.size(); ++p) {
                if (p > 0) ss << ", ";
                ss << fn.params[p].name << ": " << fn.params[p].type.toSPType();
            }
            ss << "): " << fn.returnType.toSPType() << ";\n";
        }
        ss << "}\n";
    }

    return ss.str();
}

} // namespace sp::bindgen
