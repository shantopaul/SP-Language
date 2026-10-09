#pragma once

#include "ProjectConfig.h"
#include "Lockfile.h"
#include "DependencyResolver.h"
#include "Publish.h"
#include "DependencyTree.h"
#include <string>
#include <vector>

namespace sp::spm {

struct BuildOptions {
    bool releaseMode{false};
    bool debugMode{false};
    std::string customOutput;
    std::string targetPackage; // for workspace: build only specific package member
};

struct BuildResult {
    bool success{false};
    std::string targetBinary;
    std::vector<std::string> memberBinaries;
    std::string errorMessage;
};

struct SearchResult {
    std::string name;
    std::string version;
    std::string description;
};

struct VerifyResult {
    bool success{false};
    size_t packagesVerified{0};
    std::vector<std::string> warnings;
    std::string errorMessage;
};

class BuildOrchestrator {
public:
    BuildOrchestrator() = default;

    // Verify project integrity (validates sp.toml, sp.lock, dependency graph, and package checksums)
    static VerifyResult verify(const std::string& projectDir);

    // Build the package or workspace at projectDir
    static BuildResult build(const std::string& projectDir, const BuildOptions& options = {});

    // Build multi-package workspace monorepo
    static BuildResult buildWorkspace(const std::string& workspaceDir, const BuildOptions& options = {});

    // Run the package at projectDir (builds first if needed, then executes with args)
    static int run(const std::string& projectDir, const BuildOptions& options, const std::vector<std::string>& runArgs);

    // Development server with file watcher and hot-reloading
    static int dev(const std::string& projectDir, const BuildOptions& options, const std::vector<std::string>& runArgs);

    // Clean target directory
    static bool clean(const std::string& projectDir, std::string& errorMsg);

    // Package and prepare for publishing to registry
    static PublishResult publish(const std::string& projectDir, const std::string& registryUrl = "https://registry.splang.shantopaul.com");
    static PublishResult publish(const std::string& projectDir, const PublishOptions& options);

    // Dependency tree visualization and conflict diagnostics
    static DependencyTreeResult tree(const std::string& projectDir = ".", int maxDepth = 100);

    // Search registry for packages
    static std::vector<SearchResult> search(const std::string& query, const std::string& registryUrl = "https://registry.splang.shantopaul.com");

private:
    static std::string readFile(const std::string& path);
};

} // namespace sp::spm
