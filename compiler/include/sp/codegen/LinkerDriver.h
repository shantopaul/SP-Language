#pragma once

#include <string>
#include <vector>

namespace sp::codegen {

struct LinkerDriverOptions {
    std::string targetTriple;
    std::string linkerPath;          // Custom path override for ld.lld / clang / lld-link
    std::string runtimeLibPath;      // Path to libsp_runtime.a
    std::vector<std::string> librarySearchPaths;
    std::vector<std::string> additionalLibraries;
    std::string subsystem{"console"}; // console, windows
    std::string entryPoint{"main"};
    bool stripSymbols{false};
    bool isStatic{false};
    bool verbose{false};
};

class LinkerDriver {
public:
    explicit LinkerDriver(LinkerDriverOptions options = {});

    // Links object files and runtime library directly into a native standalone executable
    [[nodiscard]] bool link(const std::vector<std::string>& objectFiles, const std::string& outputBinaryPath);

    // Convenience overload to link a single object file
    [[nodiscard]] bool linkSingle(const std::string& objectFile, const std::string& outputBinaryPath);

    // Locate the fast LLD or target platform linker
    [[nodiscard]] static std::string findLinker(const std::string& targetTriple = "", const std::string& customPath = "");

    // Locate precompiled libsp_runtime.a in project build / install / environment directories
    [[nodiscard]] static std::string findPrecompiledRuntimeLib(const std::string& customPath = "");

    [[nodiscard]] const std::string& getLastError() const noexcept { return m_lastError; }

private:
    LinkerDriverOptions m_options;
    std::string m_lastError;
};

} // namespace sp::codegen
