#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace sp::doc {

enum class DocEntryKind {
    Module,
    Function,
    Class,
    Interface,
    Struct,
    Enum,
    Variable,
    Method,
    Field
};

inline std::string docEntryKindToString(DocEntryKind kind) {
    switch (kind) {
        case DocEntryKind::Module: return "Module";
        case DocEntryKind::Function: return "Function";
        case DocEntryKind::Class: return "Class";
        case DocEntryKind::Interface: return "Interface";
        case DocEntryKind::Struct: return "Struct";
        case DocEntryKind::Enum: return "Enum";
        case DocEntryKind::Variable: return "Variable";
        case DocEntryKind::Method: return "Method";
        case DocEntryKind::Field: return "Field";
    }
    return "Unknown";
}

struct DocParam {
    std::string name;
    std::string type;
    std::string defaultValue;
    std::string description;
};

struct DocEntry {
    std::string name;
    DocEntryKind kind = DocEntryKind::Function;
    std::string signature;
    std::string docComment;
    std::string brief;
    std::string returnType;
    std::string returnDescription;
    std::vector<DocParam> params;
    std::vector<DocEntry> members; // Methods, fields, variants
    std::string visibility = "public";
    bool isStatic = false;
    bool isAsync = false;
    bool isOverride = false;
    int line = 0;
};

struct DocModule {
    std::string name;
    std::string filePath;
    std::string docComment;
    std::string brief;
    std::vector<DocEntry> entries;
};

struct DocOptions {
    std::string outputDir = "docs";
    bool generateHtml = true;
    bool generateMarkdown = true;
    std::string projectName = "SP Documentation";
};

class DocGenerator {
public:
    explicit DocGenerator(DocOptions options = {});

    DocModule extractModule(const std::string& sourceCode, const std::string& filePath = "");
    DocModule extractModuleFromFile(const std::string& filePath);
    std::vector<DocModule> extractModulesFromDirectory(const std::string& dirPath);

    std::string renderMarkdown(const DocModule& module) const;
    std::string renderHtml(const DocModule& module) const;
    std::string renderIndexMarkdown(const std::vector<DocModule>& modules) const;
    std::string renderIndexHtml(const std::vector<DocModule>& modules) const;

    bool generate(const std::string& inputPath, const std::string& outputDir = "");

    const DocOptions& getOptions() const { return m_options; }
    void setOptions(const DocOptions& options) { m_options = options; }

    static std::string cleanDocComment(const std::string& raw);
    static std::string extractBrief(const std::string& doc);

private:
    DocOptions m_options;
};

} // namespace sp::doc
