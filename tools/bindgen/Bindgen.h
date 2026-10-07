#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>

namespace sp::bindgen {

struct BindgenOptions {
    std::string moduleName{""};
    bool emitReprC{true};
    bool emitExternC{true};
    bool emitDocComments{true};
    std::vector<std::string> includeDirs;
};

struct CType {
    std::string baseType;      // e.g. "int", "char", "void", "Point"
    int pointerDepth{0};        // e.g. 0 = T, 1 = *T, 2 = **T
    bool isConst{false};
    bool isUnsigned{false};
    bool isStruct{false};

    [[nodiscard]] std::string toSPType() const;
};

struct CParam {
    std::string name;
    CType type;
};

struct CFunction {
    std::string name;
    CType returnType;
    std::vector<CParam> params;
    bool isVariadic{false};
};

struct CField {
    std::string name;
    CType type;
    int arraySize{0}; // 0 = not an array
};

struct CStruct {
    std::string name;
    bool isOpaque{false};
    std::vector<CField> fields;
};

struct CEnumVariant {
    std::string name;
    std::optional<int64_t> value;
};

struct CEnum {
    std::string name;
    std::vector<CEnumVariant> variants;
};

struct CMacroConstant {
    std::string name;
    std::string value;
    std::string spType; // "int32", "int64", "double", "string"
};

class Bindgen {
public:
    explicit Bindgen(BindgenOptions options = {});

    bool parseHeader(const std::string& headerContent);
    bool parseFile(const std::string& filePath);

    [[nodiscard]] std::string generateSP() const;

    [[nodiscard]] const std::vector<CStruct>& getStructs() const noexcept { return m_structs; }
    [[nodiscard]] const std::vector<CFunction>& getFunctions() const noexcept { return m_functions; }
    [[nodiscard]] const std::vector<CEnum>& getEnums() const noexcept { return m_enums; }
    [[nodiscard]] const std::vector<CMacroConstant>& getMacros() const noexcept { return m_macros; }
    [[nodiscard]] const std::string& getLastError() const noexcept { return m_lastError; }

private:
    BindgenOptions m_options;
    std::string m_lastError;

    std::vector<CStruct> m_structs;
    std::vector<CFunction> m_functions;
    std::vector<CEnum> m_enums;
    std::vector<CMacroConstant> m_macros;

    std::unordered_set<std::string> m_knownStructNames;
    std::unordered_set<std::string> m_knownEnumNames;
    std::unordered_set<std::string> m_knownFunctionNames;
    std::unordered_map<std::string, CType> m_typedefAliases;

    void parseTokens(const std::vector<std::string>& tokens);
    CType parseCType(size_t& idx, const std::vector<std::string>& tokens);
    void skipAttributes(size_t& idx, const std::vector<std::string>& tokens);
};

} // namespace sp::bindgen
