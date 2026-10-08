#include "BuildOrchestrator.h"
#include "Publish.h"
#include "DependencyTree.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"
#include "sp/util/PathUtils.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

namespace fs = std::filesystem;

namespace sp::spm {

std::string BuildOrchestrator::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

BuildResult BuildOrchestrator::buildWorkspace(const std::string& workspaceDir, const BuildOptions& options) {
    BuildResult result;
    fs::path tomlPath = fs::path(workspaceDir) / "sp.toml";
    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        result.errorMessage = "Failed to load workspace sp.toml: " + config.getLastError();
        return result;
    }

    if (!config.isWorkspace()) {
        result.errorMessage = "Project at '" + workspaceDir + "' is not a workspace";
        return result;
    }

    for (const auto& member : config.getWorkspaceMembers()) {
        fs::path memberDir = fs::path(workspaceDir) / member;
        BuildOptions memberOpts = options;
        memberOpts.targetPackage.clear();
        auto memberRes = build(memberDir.string(), memberOpts);
        if (!memberRes.success) {
            result.errorMessage = "Failed to build workspace member '" + member + "': " + memberRes.errorMessage;
            return result;
        }
        result.memberBinaries.push_back(memberRes.targetBinary);
    }

    result.success = true;
    if (!result.memberBinaries.empty()) {
        result.targetBinary = result.memberBinaries.back();
    }
    return result;
}

BuildResult BuildOrchestrator::build(const std::string& projectDir, const BuildOptions& options) {
    BuildResult result;
    fs::path tomlPath = fs::path(projectDir) / "sp.toml";

    if (!fs::exists(tomlPath)) {
        result.errorMessage = "Could not find 'sp.toml' in '" + projectDir + "'";
        return result;
    }

    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        result.errorMessage = "Failed to load 'sp.toml': " + config.getLastError();
        return result;
    }

    // Check if building workspace monorepo
    if (config.isWorkspace()) {
        if (!options.targetPackage.empty()) {
            for (const auto& member : config.getWorkspaceMembers()) {
                fs::path mDir = fs::path(projectDir) / member;
                ProjectConfig mCfg;
                if (mCfg.loadFromFile((mDir / "sp.toml").string()) && mCfg.getPackage().name == options.targetPackage) {
                    return build(mDir.string(), options);
                }
            }
            result.errorMessage = "Workspace member '" + options.targetPackage + "' not found in workspace";
            return result;
        }
        return buildWorkspace(projectDir, options);
    }

    // Lockfile and dependency resolution
    fs::path lockPath = fs::path(projectDir) / "sp.lock";
    Lockfile lock;
    bool needsResolution = false;

    if (!fs::exists(lockPath)) {
        needsResolution = true;
    } else {
        if (!lock.loadFromFile(lockPath.string()) || !lock.isUpToDate(config)) {
            needsResolution = true;
        }
    }

    if (needsResolution && !config.getDependencies().empty()) {
        DependencyResolver resolver;
        auto res = resolver.resolve(config, projectDir, false);
        if (!res.success) {
            result.errorMessage = "Dependency resolution failed: " + res.errorMessage;
            return result;
        }
        lock = Lockfile::fromResolution(res);
        if (!lock.saveToFile(lockPath.string())) {
            result.errorMessage = "Failed to save 'sp.lock': " + lock.getLastError();
            return result;
        }
    }

    // Determine entry file
    std::string entryRel = config.getPackage().entry;
    if (entryRel.empty()) {
        entryRel = (config.getPackage().packageType == "lib") ? "src/lib.sp" : "src/main.sp";
    }
    fs::path entryPath = fs::path(projectDir) / entryRel;
    if (!fs::exists(entryPath)) {
        result.errorMessage = "Entry point not found: " + entryPath.string();
        return result;
    }

    std::string sourceCode = readFile(entryPath.string());
    if (sourceCode.empty() && fs::file_size(entryPath) > 0) {
        result.errorMessage = "Failed to read entry file: " + entryPath.string();
        return result;
    }

    std::ostringstream diagStream;
    sp::diagnostics::DiagnosticEngine diagEngine(diagStream);

    sp::lexer::Lexer lexer(sourceCode, entryPath.string(), &diagEngine);
    auto tokens = lexer.tokenizeAll();
    if (diagEngine.hasErrors()) {
        result.errorMessage = "Lexical error during build:\n" + diagStream.str();
        return result;
    }

    sp::parser::Parser parser(tokens, entryPath.string(), &diagEngine);
    auto program = parser.parseProgram();
    if (diagEngine.hasErrors() || !program) {
        result.errorMessage = "Syntax error during build:\n" + diagStream.str();
        return result;
    }

    sp::sema::SemanticAnalyzer sema(diagEngine);
    if (!sema.analyze(*program)) {
        result.errorMessage = "Semantic error during build:\n" + diagStream.str();
        return result;
    }

    std::string modName = config.getPackage().name;
    sp::ir::IRBuilder irBuilder(sema);
    auto irModule = irBuilder.build(*program, modName);
    if (!irModule) {
        result.errorMessage = "IR generation failed for module '" + modName + "'";
        return result;
    }

    sp::codegen::CodegenOptions codegenOpts;
    codegenOpts.releaseMode = options.releaseMode;
    codegenOpts.debugSymbols = options.debugMode;
    sp::codegen::LLVMGenerator llvmGen(codegenOpts);

    fs::path targetDir = fs::path(projectDir) / "target" / (options.releaseMode ? "release" : "debug");
    std::error_code ec;
    fs::create_directories(targetDir, ec);

    bool isLib = (config.getPackage().packageType == "lib");
    std::string binName = config.getPackage().name;
    if (isLib) {
#if defined(_WIN32)
        binName += ".obj";
#else
        binName += ".o";
#endif
    } else {
#if defined(_WIN32)
        binName += ".exe";
#endif
    }

    fs::path outBin = options.customOutput.empty() ? (targetDir / binName) : fs::path(options.customOutput);

    bool buildSuccess = isLib ? llvmGen.emitObjectFile(*irModule, outBin.string())
                              : llvmGen.compileToExecutable(*irModule, outBin.string());

    if (!buildSuccess) {
        result.errorMessage = "Compilation failed: " + llvmGen.getLastError();
        if (!sp::util::hasBackendToolchain()) {
            result.errorMessage += "\n[HINT] No backend C/LLVM compiler or linker was detected on this computer.\n"
                                   "       Run 'spc setup' to auto-provision a portable toolchain.";
        }
        return result;
    }

    result.success = true;
    result.targetBinary = outBin.string();
    return result;
}

int BuildOrchestrator::run(const std::string& projectDir, const BuildOptions& options, const std::vector<std::string>& runArgs) {
    auto res = build(projectDir, options);
    if (!res.success) {
        std::cerr << "error: build failed before run:\n" << res.errorMessage << "\n";
        return 1;
    }

    fs::path absBinary = fs::absolute(res.targetBinary);
    std::string runCmd = "\"" + absBinary.string() + "\"";
    for (const auto& arg : runArgs) {
        runCmd += " \"" + arg + "\"";
    }

    int exitCode = std::system(runCmd.c_str());
    return exitCode;
}

bool BuildOrchestrator::clean(const std::string& projectDir, std::string& errorMsg) {
    fs::path targetDir = fs::path(projectDir) / "target";
    if (fs::exists(targetDir)) {
        std::error_code ec;
        fs::remove_all(targetDir, ec);
        if (ec) {
            errorMsg = "Failed to remove target directory: " + ec.message();
            return false;
        }
    }
    return true;
}

PublishResult BuildOrchestrator::publish(const std::string& projectDir, const std::string& registryUrl) {
    PublishOptions opts;
    opts.registryUrl = registryUrl;
    std::string token = PackagePublisher::resolveApiToken(opts);
    opts.dryRun = token.empty();
    return PackagePublisher::publish(projectDir, opts);
}

PublishResult BuildOrchestrator::publish(const std::string& projectDir, const PublishOptions& options) {
    return PackagePublisher::publish(projectDir, options);
}

DependencyTreeResult BuildOrchestrator::tree(const std::string& projectDir, int maxDepth) {
    fs::path tomlPath = fs::path(projectDir) / "sp.toml";
    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        DependencyTreeResult res;
        res.success = false;
        res.formattedTree = "error: could not load sp.toml in " + projectDir;
        return res;
    }
    return DependencyTree::buildTree(config, projectDir, nullptr, maxDepth);
}

std::vector<SearchResult> BuildOrchestrator::search(const std::string& query, const std::string& /*registryUrl*/) {
    // Standard package index
    static const std::vector<SearchResult> registryIndex = {
        {"sp_math", "1.0.0", "Advanced mathematics, linear algebra, and statistical functions for SP"},
        {"sp_json", "0.8.2", "High-performance JSON parser and serializer with zero-copy decoding"},
        {"sp_http", "0.4.0", "Fast HTTP client and server library with async IO support"},
        {"sp_logger", "1.2.0", "Structured terminal and file logging utility for SP applications"},
        {"sp_test", "1.0.1", "BDD-style unit and benchmark testing framework for SP"},
        {"sp_crypto", "0.3.0", "Cryptographic primitives, SHA-256, AES, and secure hashing"},
        {"sp_csv", "0.2.1", "Fast CSV reader, writer, and typed data record parser"}
    };

    std::string qLower = query;
    std::transform(qLower.begin(), qLower.end(), qLower.begin(), [](unsigned char c) { return std::tolower(c); });

    std::vector<SearchResult> matches;
    for (const auto& item : registryIndex) {
        std::string nLower = item.name;
        std::string dLower = item.description;
        std::transform(nLower.begin(), nLower.end(), nLower.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(dLower.begin(), dLower.end(), dLower.begin(), [](unsigned char c) { return std::tolower(c); });

        if (nLower.find(qLower) != std::string::npos || dLower.find(qLower) != std::string::npos) {
            matches.push_back(item);
        }
    }
    return matches;
}

VerifyResult BuildOrchestrator::verify(const std::string& projectDir) {
    VerifyResult result;

    fs::path manifestPath = fs::path(projectDir) / "sp.toml";
    if (!fs::exists(manifestPath)) {
        result.errorMessage = "Missing manifest 'sp.toml' in directory: " + projectDir;
        return result;
    }

    ProjectConfig config;
    if (!config.loadFromFile(manifestPath.string())) {
        result.errorMessage = "Failed to parse 'sp.toml': " + config.getLastError();
        return result;
    }

    // Verify root package metadata
    if (config.getPackage().name.empty()) {
        result.errorMessage = "Package name cannot be empty in sp.toml";
        return result;
    }

    auto rootVersion = Version::parse(config.getPackage().version);
    if (!rootVersion) {
        result.errorMessage = "Invalid package version '" + config.getPackage().version + "' in sp.toml";
        return result;
    }

    fs::path lockPath = fs::path(projectDir) / "sp.lock";
    if (!fs::exists(lockPath)) {
        if (config.getDependencies().empty()) {
            result.success = true;
            result.packagesVerified = 0;
            return result;
        }
        result.errorMessage = "Missing lockfile 'sp.lock'. Run 'spm install' to resolve dependencies.";
        return result;
    }

    Lockfile lock;
    if (!lock.loadFromFile(lockPath.string())) {
        result.errorMessage = "Failed to load 'sp.lock': " + lock.getLastError();
        return result;
    }

    // Verify lockfile satisfies config
    if (!lock.isUpToDate(config)) {
        result.errorMessage = "sp.lock is out of sync with sp.toml. Run 'spm install' or 'spm update'.";
        return result;
    }

    // Verify each locked package's integrity & cryptographic checksum
    std::vector<std::string> seen;
    for (const auto& pkg : lock.getPackages()) {
        if (pkg.name.empty()) {
            result.errorMessage = "Found locked package with empty name";
            return result;
        }

        for (const auto& s : seen) {
            if (s == pkg.name) {
                result.errorMessage = "Duplicate package '" + pkg.name + "' detected in sp.lock";
                return result;
            }
        }
        seen.push_back(pkg.name);

        auto ver = Version::parse(pkg.version);
        if (!ver) {
            result.errorMessage = "Invalid version '" + pkg.version + "' for package '" + pkg.name + "' in sp.lock";
            return result;
        }

        // Validate cryptographic checksum
        if (pkg.checksum.empty()) {
            result.errorMessage = "Security audit failed: Missing checksum for package '" + pkg.name + "' in sp.lock";
            return result;
        }

        // Verify checksum matches expected deterministic hash
        std::string expectedChecksum = DependencyResolver::computeChecksum(pkg.name, *ver, pkg.source);
        if (pkg.checksum != expectedChecksum) {
            result.errorMessage = "Security audit failed: Checksum mismatch / tampering detected for package '" +
                                  pkg.name + "' (expected: " + expectedChecksum + ", recorded: " + pkg.checksum + ")";
            return result;
        }

        result.packagesVerified++;
    }

    result.success = true;
    return result;
}

} // namespace sp::spm

