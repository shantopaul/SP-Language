#pragma once

#include "ProjectConfig.h"
#include "Lockfile.h"
#include "DependencyResolver.h"
#include <string>
#include <vector>

namespace sp::spm {

struct BuildOptions {
    bool releaseMode{false};
    bool debugMode{false};
    std::string customOutput;
};

struct BuildResult {
    bool success{false};
    std::string targetBinary;
    std::string errorMessage;
};

struct PublishResult {
    bool success{false};
    std::string packageArtifact;
    std::string errorMessage;
};

struct SearchResult {
    std::string name;
    std::string version;
    std::string description;
};

class BuildOrchestrator {
public:
    BuildOrchestrator() = default;

    // Build the package at projectDir (resolves dependencies, generates IR, compiles binary to target/debug or target/release)
    static BuildResult build(const std::string& projectDir, const BuildOptions& options = {});

    // Run the package at projectDir (builds first if needed, then executes with args)
    static int run(const std::string& projectDir, const BuildOptions& options, const std::vector<std::string>& runArgs);

    // Clean target directory
    static bool clean(const std::string& projectDir, std::string& errorMsg);

    // Package and prepare for publishing to registry
    static PublishResult publish(const std::string& projectDir, const std::string& registryUrl = "https://registry.splang.shantopaul.com");

    // Search registry for packages
    static std::vector<SearchResult> search(const std::string& query, const std::string& registryUrl = "https://registry.splang.shantopaul.com");

private:
    static std::string readFile(const std::string& path);
};

} // namespace sp::spm
