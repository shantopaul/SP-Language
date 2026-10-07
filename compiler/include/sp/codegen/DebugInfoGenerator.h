#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sp::codegen {

struct DebugLocation {
    uint32_t line{1};
    uint32_t column{1};
    int subprogramId{-1};
};

struct SubprogramInfo {
    int id{0};
    std::string name;
    std::string linkageName;
    uint32_t line{1};
    uint32_t scopeLine{1};
    int fileId{0};
};

class DebugInfoGenerator {
public:
    DebugInfoGenerator(std::string sourceFilename = "main.sp", std::string sourceDirectory = ".");

    // Configure primary source file and directory
    void setSourceFile(std::string filename, std::string directory);

    // Register a function/method subprogram for debug scope
    int createSubprogram(const std::string& name, const std::string& linkageName, uint32_t line);

    // Allocate a debug location ID (!dbg !<id>) for a source line/column
    int createLocation(uint32_t line, uint32_t column, int subprogramId);

    // Generates LLVM IR module-level debug metadata flags and compile unit
    [[nodiscard]] std::string generateMetadataProlog() const;

    // Generates LLVM IR trailing numbered metadata nodes (!0 = !DIFile(...), etc.)
    [[nodiscard]] std::string generateMetadataEpilog() const;

    // Injects debug metadata and !dbg tags into an LLVM IR text module
    [[nodiscard]] std::string injectDebugInfo(const std::string& llvmIR);

    [[nodiscard]] const std::string& getSourceFilename() const noexcept { return m_sourceFilename; }
    [[nodiscard]] const std::string& getSourceDirectory() const noexcept { return m_sourceDirectory; }

private:
    std::string m_sourceFilename;
    std::string m_sourceDirectory;
    std::vector<SubprogramInfo> m_subprograms;
    std::vector<DebugLocation> m_locations;
};

} // namespace sp::codegen
