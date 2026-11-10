#include "bindgen/Bindgen.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace sp::bindgen {

namespace fs = std::filesystem;

bool isIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Phase 39 slice C: an identifier that looks like a C macro name
// (all-caps, e.g. FAR, ZEXPORT, ZEXTERN) as opposed to a type name.
bool isAllCapsIdentifier(const std::string& tok) {
    if (tok.size() < 2 || !std::isupper(static_cast<unsigned char>(tok[0]))) return false;
    for (char c : tok) {
        if (!(std::isupper(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)) || c == '_')) {
            return false;
        }
    }
    return true;
}

// Phase 39 slice C: tokens that can start a C type.
bool isTypeStartToken(const std::string& tok) {
    static const std::unordered_set<std::string> kTypeStarts = {
        "const", "volatile", "unsigned", "signed", "struct", "enum",
        "void", "char", "short", "int", "long", "float", "double",
        "bool", "_Bool",
    };
    return kTypeStarts.count(tok) != 0;
}

std::vector<std::string> tokenizeC(const std::string& input, std::vector<CMacroConstant>& outMacros,
                                   std::vector<std::pair<std::string, std::string>>& outTypeAliases) {
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
                        // Phase 39 slice C: #ifdef/#else alternative
                        // definitions (zlib's MAX_MEM_LEVEL 8/9) would
                        // emit duplicate `const`s. C's last-#define-wins
                        // rule: drop any earlier entry with this name.
                        outMacros.erase(std::remove_if(outMacros.begin(), outMacros.end(),
                                                       [&](const CMacroConstant& m) {
                                                           return m.name == macroName;
                                                       }),
                                        outMacros.end());
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
                            } else if (!val.empty() &&
                                       std::all_of(val.begin(), val.end(), [](char ch) {
                                           return isIdentifierChar(ch);
                                       })) {
                                // Phase 39 slice C: `#define z_off64_t
                                // off64_t` — a macro aliasing a type
                                // name, not a value constant. Recorded
                                // in the separate #define-alias map
                                // (a real typedef always wins at
                                // resolve time; see Bindgen.h).
                                outTypeAliases.push_back({macroName, val});
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

std::string CType::toSPType() const {
    std::string base = baseType;
    int depth = pointerDepth;
    // Phase 39 slice C: va_list comes from <stdarg.h>, which is not
    // always on the include path (zlib's gzvprintf). On System V it
    // is a struct array passed by pointer, so it is `ptr<void>`.
    if (base == "va_list") {
        base = "void";
        depth += 1;
    }
    if (base == "void") {
        if (depth == 0) return "void";
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
    } else if (base == "ssize_t" || base == "intptr_t" || base == "ptrdiff_t" ||
               base == "off_t" || base == "off64_t") {
        // Phase 39 slice C: off_t/off64_t are `long` on LP64.
        base = "int64";
    } else if (base == "float") {
        base = "float";
    } else if (base == "double") {
        base = "double";
    } else if (base == "bool" || base == "_Bool") {
        base = "bool";
    }

    std::string result = base;
    for (int p = 0; p < depth; ++p) {
        result = "ptr<" + result + ">";
    }
    return result;
}

Bindgen::Bindgen(BindgenOptions options)
    : m_options(std::move(options)) {}

bool Bindgen::parseFile(const std::string& filePath) {
    m_structs.clear();
    m_functions.clear();
    m_enums.clear();
    m_macros.clear();
    m_knownStructNames.clear();
    m_knownEnumNames.clear();
    m_knownFunctionNames.clear();
    m_typedefAliases.clear();
    m_visitedFiles.clear();
    return parseFileRecursive(filePath);
}

bool Bindgen::parseFileRecursive(const std::string& filePath) {
    // Phase 39 slice C: #include following. Each file is parsed
    // once (visited set), includes first so their typedefs resolve
    // when the including file uses them.
    std::error_code ec;
    std::string key = fs::weakly_canonical(fs::absolute(filePath, ec), ec).string();
    if (key.empty()) key = filePath;
    if (!m_visitedFiles.insert(key).second) return true;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_lastError = "Could not open C header file: " + filePath;
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    fs::path dir = fs::path(filePath).parent_path();
    std::istringstream lines(content);
    std::string line;
    while (std::getline(lines, line)) {
        size_t pos = line.find_first_not_of(" \t");
        if (pos == std::string::npos || line.compare(pos, 8, "#include") != 0) continue;
        size_t start = line.find_first_of("\"<", pos + 8);
        if (start == std::string::npos) continue;
        char closer = line[start] == '"' ? '"' : '>';
        size_t end = line.find(closer, start + 1);
        if (end == std::string::npos) continue;
        std::string inc = line.substr(start + 1, end - start - 1);
        if (inc.empty()) continue;

        // Quoted includes resolve against the including file's
        // directory first; angled ones against the include dirs.
        std::vector<fs::path> candidates;
        if (line[start] == '"') candidates.push_back(dir / inc);
        for (const auto& d : m_options.includeDirs) candidates.push_back(fs::path(d) / inc);
        if (line[start] == '"') {
            for (const auto& d : m_options.includeDirs) candidates.push_back(fs::path(d) / inc);
        }
        for (const auto& cand : candidates) {
            if (fs::exists(cand)) {
                if (!parseFileRecursive(cand.string())) return false;
                break;
            }
        }
        // An unresolvable include is skipped silently: system
        // headers a header mentions but does not need (e.g. a
        // platform-only include) must not fail the whole bindgen.
    }

    return parseContent(content);
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
    m_visitedFiles.clear();

    return parseContent(headerContent);
}

bool Bindgen::parseContent(const std::string& headerContent) {
    std::vector<CMacroConstant> extractedMacros;
    std::vector<std::pair<std::string, std::string>> extractedTypeAliases;
    std::vector<std::string> tokens = tokenizeC(headerContent, extractedMacros, extractedTypeAliases);
    m_macros.insert(m_macros.end(), extractedMacros.begin(), extractedMacros.end());
    // Phase 39 slice C: `#define NAME <type>` aliases (zlib's
    // z_off64_t) go to the SEPARATE #define map (see Bindgen.h): a
    // real typedef always wins at resolve time, whatever the file
    // order. Last write wins within this map.
    for (const auto& [alias, underlying] : extractedTypeAliases) {
        CType type;
        type.baseType = underlying;
        m_defineTypeAliases[alias] = type;
    }

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

CType Bindgen::resolveTypedef(CType type) const {
    std::unordered_set<std::string> seen;
    while (!type.baseType.empty() && !seen.count(type.baseType)) {
        seen.insert(type.baseType);
        // Phase 39 slice C: real typedefs win over `#define` aliases
        // (zlib's inactive-#ifdef Z_PREFIX renames must never shadow
        // a real typedef from an earlier file).
        const CType* alias = nullptr;
        auto it = m_typedefAliases.find(type.baseType);
        if (it != m_typedefAliases.end()) {
            alias = &it->second;
        } else if (!m_knownStructNames.count(type.baseType) &&
                   !m_knownEnumNames.count(type.baseType)) {
            // Phase 39 slice C: a known struct/enum name is the truth
            // — an inactive-#ifdef rename (`#define gz_header
            // z_gz_header`) must not rename it.
            auto dit = m_defineTypeAliases.find(type.baseType);
            if (dit != m_defineTypeAliases.end()) alias = &dit->second;
        }
        if (!alias) break;
        CType merged;
        merged.baseType = alias->baseType;
        merged.pointerDepth = type.pointerDepth + alias->pointerDepth;
        merged.isConst = type.isConst || alias->isConst;
        merged.isUnsigned = type.isUnsigned || alias->isUnsigned;
        merged.isStruct = alias->isStruct || type.isStruct;
        type = merged;
    }
    return type;
}

// Phase 39 slice C: parse `(*name)(params)` at tokens[idx] == "(".
// The signature is discarded (SP has no function-type syntax); the
// declarator name is returned and the caller uses `ptr<void>`.
bool Bindgen::parseFnPointerDeclarator(size_t& idx, const std::vector<std::string>& tokens,
                                      std::string& outName) {
    size_t n = tokens.size();
    if (idx + 1 >= n || tokens[idx] != "(" || tokens[idx + 1] != "*") return false;
    size_t i = idx + 2;
    skipAttributes(i, tokens);
    if (i >= n || !isIdentifierChar(tokens[i][0])) return false;
    outName = tokens[i++];
    if (i >= n || tokens[i] != ")") return false;
    i++; // consume )
    if (i >= n || tokens[i] != "(") return false;
    // Skip the parameter list (balanced parens).
    int depth = 0;
    while (i < n) {
        if (tokens[i] == "(") depth++;
        else if (tokens[i] == ")") {
            depth--;
            if (depth == 0) { i++; break; }
        }
        i++;
    }
    idx = i;
    return true;
}

CType Bindgen::parseCType(size_t& idx, const std::vector<std::string>& tokens) {
    CType type;
    skipAttributes(idx, tokens);

    // Phase 39 slice C: macro prefixes like `z_const` in
    // `z_const Bytef *p` — a non-keyword identifier followed by a
    // type and then `*` cannot be a type name (a type is followed
    // by a name, not by type + `*`), so it is a macro expanding to
    // const/nothing. Skip it (possibly chained). The discriminator
    // is that the token after the candidate (skipping qualifiers)
    // resolves to a real type: `z_stream FAR *z_streamp` has the
    // same shape, but FAR is not a type — there `z_stream` IS the
    // type and must not be skipped. Likewise `Byte const *voidpc`
    // must not skip Byte: `const` is a qualifier, not a type.
    static const std::unordered_set<std::string> kTypeKeywords = {
        "const", "volatile", "unsigned", "signed", "struct", "enum",
        "void", "char", "short", "int", "long", "float", "double",
        "bool", "_Bool",
    };
    static const std::unordered_set<std::string> kPrimitiveTypes = {
        "void", "char", "short", "int", "long", "float", "double",
        "bool", "_Bool",
    };
    auto isKnownTypeToken = [&](const std::string& tok) {
        return kPrimitiveTypes.count(tok) || tok == "struct" || tok == "enum" ||
               m_typedefAliases.count(tok) || m_knownStructNames.count(tok);
    };
    auto skipQualifiers = [&](size_t j) {
        while (j < tokens.size() &&
               (tokens[j] == "const" || tokens[j] == "volatile" ||
                tokens[j] == "unsigned" || tokens[j] == "signed")) {
            j++;
        }
        return j;
    };
    while (idx + 1 < tokens.size() && isIdentifierChar(tokens[idx][0]) &&
           !kTypeKeywords.count(tokens[idx])) {
        size_t j = skipQualifiers(idx + 1);
        if (j >= tokens.size() || !isKnownTypeToken(tokens[j])) break;
        // After the type, allow convention macros, then require `*`.
        size_t k = j + 1;
        while (k < tokens.size() && isAllCapsIdentifier(tokens[k])) k++;
        if (k >= tokens.size() || tokens[k] != "*") break;
        idx++;
    }

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
        } else if (isAllCapsIdentifier(tok) && idx + 1 < tokens.size() &&
                   (isTypeStartToken(tokens[idx + 1]) || tokens[idx + 1] == "*" ||
                    isAllCapsIdentifier(tokens[idx + 1]) ||
                    m_typedefAliases.count(tokens[idx + 1]) ||
                    m_defineTypeAliases.count(tokens[idx + 1]) ||
                    m_knownStructNames.count(tokens[idx + 1]))) {
            // Phase 39 slice C: empty/convention macros (FAR,
            // ZEXPORT, ZEXTERN) before a type. A real all-caps
            // typedef name is (almost) never skipped: it is followed
            // by a declarator name, not by a type-start/`*`/macro/
            // known-type. The exception is `ZEXTERN uLong ...`,
            // where the macro precedes a typedef-named type — that
            // is exactly the case this arm handles.
            idx++;
        } else {
            break;
        }
    }

    // Phase 39 slice C: read the base type name. The old code took
    // whatever token followed, which misread `unsigned *pending`
    // (took `*` as the type) and `unsigned len` (took the parameter
    // NAME as the type). A qualifier with no type word after it
    // means plain `int` (C rule); anything else must be a primitive
    // word or a real identifier.
    if (type.baseType.empty() && idx < tokens.size()) {
        std::string base = tokens[idx];
        if (kPrimitiveTypes.count(base)) {
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
        } else if (!hasUnsigned && !hasSigned && isIdentifierChar(base[0])) {
            // A named type (typedef / struct name).
            idx++;
            type.baseType = base;
        } else if (hasUnsigned || hasSigned) {
            type.baseType = "int";
        }
        // Otherwise (e.g. `*` with no base): leave baseType empty;
        // the pointer loop below handles the `*`.
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
        } else if (isAllCapsIdentifier(tokens[idx])) {
            // Phase 39 slice C: empty macro between a type and `*`
            // or an alias (`typedef Byte FAR Bytef;`).
            idx++;
        } else {
            break;
        }
    }

    // Phase 39 slice C: resolve typedefs at the use site
    // (z_streamp -> ptr<z_stream>, Bytef -> uint8, ...).
    return resolveTypedef(type);
}

void Bindgen::parseStructFields(size_t& idx, const std::vector<std::string>& tokens,
                                CStruct& out, const std::string& outerName) {
    // idx at "{"; ends past "}".
    size_t n = tokens.size();
    idx++; // consume {
    while (idx < n && tokens[idx] != "}") {
        // Phase 39 slice C: anonymous struct/union member.
        if (tokens[idx] == "struct" || tokens[idx] == "union") {
            size_t save = idx;
            idx++;
            std::string tag;
            if (idx < n && tokens[idx] != "{" && tokens[idx] != ";") tag = tokens[idx++];
            if (idx < n && tokens[idx] == "{" && tag.empty()) {
                CStruct inner;
                parseStructFields(idx, tokens, inner, outerName);
                if (idx < n && isIdentifierChar(tokens[idx][0])) {
                    // Named field of anonymous struct/union type:
                    // synthesize a struct for it.
                    std::string fieldName = tokens[idx++];
                    std::string synthName = outerName + "_" + fieldName;
                    CStruct synth;
                    synth.name = synthName;
                    synth.fields = inner.fields;
                    if (!m_knownStructNames.count(synthName)) {
                        m_knownStructNames.insert(synthName);
                        m_structs.push_back(synth);
                    }
                    CType ft;
                    ft.baseType = synthName;
                    ft.isStruct = true;
                    out.fields.push_back({fieldName, ft, 0});
                } else {
                    // Unnamed: C anonymous members sit at the outer
                    // offset, so flatten the inner fields.
                    for (const auto& f : inner.fields) out.fields.push_back(f);
                }
                if (idx < n && tokens[idx] == ";") idx++;
                continue;
            }
            // Not anonymous (has a tag, or no brace group): rewind
            // and fall through to the normal field parse.
            idx = save;
        }
        CType fieldType = parseCType(idx, tokens);
        // Phase 39 slice C: function-pointer field `Ret (*name)(params)`.
        if (idx < n && tokens[idx] == "(") {
            std::string fpName;
            if (parseFnPointerDeclarator(idx, tokens, fpName)) {
                CType fpType;
                fpType.baseType = "void";
                fpType.pointerDepth = 1;
                out.fields.push_back({fpName, fpType, 0});
                if (idx < n && tokens[idx] == ";") idx++;
                continue;
            }
        }
        if (idx < n && isIdentifierChar(tokens[idx][0])) {
            std::string fieldName = tokens[idx++];
            int arrSize = 0;
            if (idx < n && tokens[idx] == "[") {
                idx++; // [
                if (idx < n && std::isdigit(static_cast<unsigned char>(tokens[idx][0]))) {
                    arrSize = std::stoi(tokens[idx++]);
                }
                if (idx < n && tokens[idx] == "]") idx++;
                fieldType.pointerDepth++; // treat array as pointer in SP struct
            }
            out.fields.push_back({fieldName, fieldType, arrSize});
        }
        if (idx < n && tokens[idx] == ";") idx++;
    }
    if (idx < n && tokens[idx] == "}") idx++;
}

void Bindgen::parseTokens(const std::vector<std::string>& tokens) {
    size_t i = 0;
    size_t n = tokens.size();

    while (i < n) {
        // Phase 39 slice C: `extern "C" {` (from `#ifdef __cplusplus`
        // guards) is not a declaration — drop the opener. This MUST
        // be checked before skipAttributes, which eats a bare
        // `extern` on its own (and then the function-case error
        // recovery swallows the first real declaration after it —
        // zlib lost `alloc_func` this way). A stray `}` at statement
        // level is the block's closer; struct/enum bodies are
        // consumed by their own parsers, so a bare `}` here is never
        // a real declaration.
        if (i + 2 < n && tokens[i] == "extern" && tokens[i + 1].size() >= 2 &&
            tokens[i + 1][0] == '"' && tokens[i + 2] == "{") {
            i += 3;
            continue;
        }
        skipAttributes(i, tokens);
        if (i >= n) break;

        const std::string& tok = tokens[i];

        if (tok == "}") {
            i++;
            continue;
        }

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
                    CStruct st;
                    st.name = tag;
                    parseStructFields(i, tokens, st, tag.empty() ? "Anon" : tag);

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
                } else if (i < n && tokens[i] == "*") {
                    // Phase 39 slice C: pointer typedef (the
                    // `struct * {}` bug): typedef struct Tag *Alias;
                    i++; // consume *
                    skipAttributes(i, tokens);
                    if (i < n && isIdentifierChar(tokens[i][0])) {
                        std::string alias = tokens[i++];
                        CType underlying;
                        underlying.baseType = tag;
                        underlying.pointerDepth = 1;
                        underlying.isStruct = true;
                        m_typedefAliases[alias] = underlying;
                    }
                    while (i < n && tokens[i] != ";") i++;
                    if (i < n && tokens[i] == ";") i++;
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
                // Phase 39 slice C: function-pointer typedef
                // `typedef Ret (*Name)(params);` — the name resolves
                // to `ptr<void>` at use sites (SP has no fn type).
                if (i < n && tokens[i] == "(") {
                    std::string fpName;
                    if (parseFnPointerDeclarator(i, tokens, fpName)) {
                        CType fpType;
                        fpType.baseType = "void";
                        fpType.pointerDepth = 1;
                        m_typedefAliases[fpName] = fpType;
                        while (i < n && tokens[i] != ";") i++;
                        if (i < n && tokens[i] == ";") i++;
                        continue;
                    }
                }
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
            size_t checkpoint = i;
            i++;
            if (i < n && isIdentifierChar(tokens[i][0])) {
                std::string stName = tokens[i++];
                if (i < n && tokens[i] == "{") {
                    CStruct st;
                    st.name = stName;
                    parseStructFields(i, tokens, st, stName);
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
                // Phase 39 slice C: `struct Name fn(...)` — a
                // struct-typed function (struct return). Rewind so
                // the function parser sees the full declaration.
                i = checkpoint;
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
        // Phase 39 slice C: skip a convention macro between the
        // return type and the function name (ZEXPORT compress). A
        // real all-caps function name is never eaten: it is followed
        // by `(`, not by an identifier.
        while (i + 1 < n && isAllCapsIdentifier(tokens[i]) && isIdentifierChar(tokens[i + 1][0])) {
            i++;
        }

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
                    // Phase 39 slice C: function-pointer parameter
                    // `Ret (*name)(params)` -> ptr<void>.
                    if (i < n && tokens[i] == "(") {
                        std::string fpName;
                        if (parseFnPointerDeclarator(i, tokens, fpName)) {
                            CType fpType;
                            fpType.baseType = "void";
                            fpType.pointerDepth = 1;
                            fn.params.push_back({fpName, fpType});
                            paramIdx++;
                            if (i < n && tokens[i] == ",") i++;
                            continue;
                        }
                    }
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
            // Phase 39 slice C: emit the recorded variadic marker
            // (the SP parser accepts `...` as the last parameter).
            if (fn.isVariadic) {
                if (!fn.params.empty()) ss << ", ";
                ss << "...";
            }
            ss << "): " << fn.returnType.toSPType() << ";\n";
        }
        ss << "}\n";
    }

    return ss.str();
}

} // namespace sp::bindgen
