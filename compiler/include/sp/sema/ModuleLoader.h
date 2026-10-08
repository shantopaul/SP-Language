#pragma once

#include "sp/ast/Declaration.h"
#include "sp/ast/Edition.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <memory>

namespace sp::sema {

class ModuleLoader {
public:
    explicit ModuleLoader(diagnostics::DiagnosticEngine& diagnostics);

    // Search path management
    void addSearchPath(const std::filesystem::path& path);
    void setStdlibPath(const std::filesystem::path& path);
    [[nodiscard]] const std::filesystem::path& getStdlibPath() const noexcept { return m_stdlibPath; }

    void setEdition(ast::Edition edition) noexcept { m_edition = edition; }
    [[nodiscard]] ast::Edition getEdition() const noexcept { return m_edition; }

    // Resolve an import specifier (e.g. "std.http", "routes.users") from a referencing file
    std::filesystem::path resolveModulePath(const std::string& importSpecifier, 
                                            const std::filesystem::path& referencingFile);

    // Load, tokenize, and parse a single module file into an AST Program
    std::shared_ptr<ast::Program> loadModule(const std::filesystem::path& moduleFilePath);

    // Recursively resolve all imports starting from a root program, detecting circular dependencies
    // and merging all imported declarations into rootProgram in topological dependency order.
    bool loadAndMergeAll(ast::Program& rootProgram, const std::filesystem::path& rootFilePath);

    // Access loaded module cache
    [[nodiscard]] const std::unordered_map<std::string, std::shared_ptr<ast::Program>>& getLoadedModules() const noexcept {
        return m_loadedModules;
    }

private:
    diagnostics::DiagnosticEngine& m_diagnostics;
    std::filesystem::path m_stdlibPath;
    std::vector<std::filesystem::path> m_searchPaths;
    ast::Edition m_edition{ast::Edition::Edition2026};

    // Cache of loaded ASTs indexed by canonical path string
    std::unordered_map<std::string, std::shared_ptr<ast::Program>> m_loadedModules;

    // Set of canonical paths already merged to avoid duplicate declarations
    std::unordered_set<std::string> m_mergedPaths;

    // Active stack for circular import detection
    std::vector<std::string> m_importStack;

    std::string readFile(const std::filesystem::path& path);
};

} // namespace sp::sema
