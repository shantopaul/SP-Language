#include "sp/sema/ModuleLoader.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/util/PathUtils.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace sp::sema {

ModuleLoader::ModuleLoader(diagnostics::DiagnosticEngine& diagnostics)
    : m_diagnostics(diagnostics),
      m_stdlibPath(util::getStdlibDir()),
      m_edition(ast::Edition::Edition2026) {
    // Default search paths include current working directory and project root / src
    m_searchPaths.push_back(fs::current_path());
    if (fs::exists(fs::current_path() / "src")) {
        m_searchPaths.push_back(fs::current_path() / "src");
    }
}

void ModuleLoader::addSearchPath(const fs::path& path) {
    if (!path.empty() && fs::exists(path)) {
        m_searchPaths.push_back(fs::absolute(path));
    }
}

void ModuleLoader::setStdlibPath(const fs::path& path) {
    if (!path.empty() && fs::exists(path)) {
        m_stdlibPath = fs::absolute(path);
    }
}

std::string ModuleLoader::readFile(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

fs::path ModuleLoader::resolveModulePath(const std::string& importSpecifier, 
                                        const fs::path& referencingFile) {
    if (importSpecifier.empty()) return {};

    // 1. Standard library imports: e.g. "std.http", "std.json", "std.net", "std.crypto"
    if (importSpecifier.rfind("std.", 0) == 0) {
        std::string sub = importSpecifier.substr(4);
        std::replace(sub.begin(), sub.end(), '.', '/');

        if (!m_stdlibPath.empty() && fs::exists(m_stdlibPath)) {
            // Priority a: stdlib/<subpath>/<subpath>.sp (e.g. stdlib/http/http.sp)
            fs::path p1 = m_stdlibPath / sub / (fs::path(sub).filename().string() + ".sp");
            if (fs::exists(p1)) return fs::absolute(p1);

            // Priority b: stdlib/<subpath>.sp (e.g. stdlib/io.sp)
            fs::path p2 = m_stdlibPath / (sub + ".sp");
            if (fs::exists(p2)) return fs::absolute(p2);

            // Priority c: stdlib/<subpath>/mod.sp
            fs::path p3 = m_stdlibPath / sub / "mod.sp";
            if (fs::exists(p3)) return fs::absolute(p3);
        }
    }

    // 2. Relative or project module imports: e.g. "routes.users", "models.user", "./helper"
    std::string relStr = importSpecifier;
    std::replace(relStr.begin(), relStr.end(), '.', '/');
    std::string lastComp = fs::path(relStr).filename().string();

    fs::path refDir = referencingFile.empty() ? fs::current_path() : referencingFile.parent_path();

    // Try relative to referencing file's directory
    fs::path cand1 = refDir / (relStr + ".sp");
    if (fs::exists(cand1)) return fs::absolute(cand1);

    fs::path cand2 = refDir / relStr / (lastComp + ".sp");
    if (fs::exists(cand2)) return fs::absolute(cand2);

    fs::path cand3 = refDir / relStr / "mod.sp";
    if (fs::exists(cand3)) return fs::absolute(cand3);

    // Try in search paths
    for (const auto& spath : m_searchPaths) {
        fs::path p1 = spath / (relStr + ".sp");
        if (fs::exists(p1)) return fs::absolute(p1);

        fs::path p2 = spath / relStr / (lastComp + ".sp");
        if (fs::exists(p2)) return fs::absolute(p2);

        fs::path p3 = spath / relStr / "mod.sp";
        if (fs::exists(p3)) return fs::absolute(p3);
    }

    return {};
}

std::shared_ptr<ast::Program> ModuleLoader::loadModule(const fs::path& moduleFilePath) {
    std::error_code ec;
    fs::path canon = fs::canonical(moduleFilePath, ec);
    if (ec) canon = fs::absolute(moduleFilePath);

    std::string canonStr = canon.string();
    auto it = m_loadedModules.find(canonStr);
    if (it != m_loadedModules.end()) {
        return it->second;
    }

    std::string source = readFile(canon);
    if (source.empty() && fs::exists(canon) && fs::file_size(canon) > 0) {
        m_diagnostics.reportError("E1009", "Failed to read module source file: " + canonStr,
                                  diagnostics::SourceLocation(canonStr, 1, 1));
        return nullptr;
    }

    lexer::Lexer lexer(source, canonStr, &m_diagnostics);
    auto tokens = lexer.tokenizeAll();
    if (m_diagnostics.hasErrors()) {
        return nullptr;
    }

    parser::Parser parser(tokens, canonStr, &m_diagnostics);
    parser.setEdition(m_edition);
    auto program = parser.parseProgram();
    if (m_diagnostics.hasErrors() || !program) {
        return nullptr;
    }

    std::shared_ptr<ast::Program> sharedProg = std::move(program);
    m_loadedModules[canonStr] = sharedProg;
    return sharedProg;
}

bool ModuleLoader::loadAndMergeAll(ast::Program& rootProgram, const fs::path& rootFilePath) {
    std::error_code ec;
    fs::path rootCanon = fs::canonical(rootFilePath, ec);
    if (ec) rootCanon = fs::absolute(rootFilePath);

    std::string rootCanonStr = rootCanon.string();
    fs::path rootDir = rootCanon.parent_path();
    addSearchPath(rootDir);
    if (fs::exists(rootDir / "src")) {
        addSearchPath(rootDir / "src");
    }
    m_importStack.push_back(rootCanonStr);
    m_mergedPaths.insert(rootCanonStr);

    std::vector<std::shared_ptr<ast::Program>> dependencies;

    // Scan for import declarations in rootProgram
    for (const auto& decl : rootProgram.getDeclarations()) {
        if (decl && decl->getKind() == ast::ASTNodeKind::ImportDecl) {
            const auto* importDecl = static_cast<const ast::ImportDecl*>(decl.get());
            std::string spec = importDecl->getPath();
            if (spec.empty()) continue;

            // Known pure built-in modules that are natively satisfied by compiler symbol table and IR lowering
            if (spec == "std.io" || spec == "std.math" || spec == "std.fs" ||
                spec == "std.time" || spec == "std.os" || spec == "std.process" ||
                spec == "std.string" || spec == "std.mem" || spec == "std.sync") {
                continue;
            }

            fs::path modPath = resolveModulePath(spec, rootCanon);

            if (modPath.empty() || !fs::exists(modPath)) {
                m_diagnostics.reportError("E1009", "Module '" + spec + "' could not be resolved from path",
                                          importDecl->getLocation(),
                                          "Ensure the file exists in your project or stdlib directory.");
                continue;
            }

            fs::path modCanon = fs::canonical(modPath, ec);
            if (ec) modCanon = fs::absolute(modPath);
            std::string modCanonStr = modCanon.string();

            // Circular dependency check
            auto stackIt = std::find(m_importStack.begin(), m_importStack.end(), modCanonStr);
            if (stackIt != m_importStack.end()) {
                std::string cyclePath;
                for (auto it = stackIt; it != m_importStack.end(); ++it) {
                    cyclePath += fs::path(*it).filename().string() + " -> ";
                }
                cyclePath += modCanon.filename().string();

                m_diagnostics.reportError("E1010", "Circular import detected: " + cyclePath,
                                          importDecl->getLocation());
                continue;
            }

            // If not already merged, recursively load and merge it
            if (m_mergedPaths.find(modCanonStr) == m_mergedPaths.end()) {
                auto importedModule = loadModule(modCanon);
                if (importedModule) {
                    // Recursively resolve the imported module's dependencies first
                    if (!loadAndMergeAll(*importedModule, modCanon)) {
                        continue;
                    }
                    dependencies.push_back(importedModule);
                }
            }
        }
    }

    m_importStack.pop_back();

    // Merge declarations: dependencies are prepended before root declarations
    if (!dependencies.empty()) {
        std::vector<ast::ASTNodePtr> mergedDecls;

        for (auto& dep : dependencies) {
            for (auto& depDecl : dep->getDeclarations()) {
                // Do not propagate ModuleDecl or ImportDecl into merged scope
                if (depDecl && depDecl->getKind() != ast::ASTNodeKind::ModuleDecl &&
                    depDecl->getKind() != ast::ASTNodeKind::ImportDecl) {
                    mergedDecls.push_back(std::move(depDecl));
                }
            }
        }

        // Add root declarations
        for (auto& rootDecl : rootProgram.getDeclarations()) {
            mergedDecls.push_back(std::move(rootDecl));
        }

        rootProgram.getDeclarations() = std::move(mergedDecls);
    }

    return !m_diagnostics.hasErrors();
}

} // namespace sp::sema
