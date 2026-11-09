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

// Result of `spm install` (Phase 4 slice A).
struct InstallResult {
    bool success{false};
    size_t packagesInstalled{0}; // registry packages vendored into .spm/packages
    bool registryUnavailable{false}; // resolved from cache only (offline)
    std::string errorMessage;
};

class BuildOrchestrator {
public:
    BuildOrchestrator() = default;

    // Registry base URL: explicit arg > SP_REGISTRY_URL env > http://127.0.0.1:8090.
    static std::string defaultRegistryUrl();

    // Install (resolve + lock + vendor) the dependencies of the project
    // at projectDir. forceResolve re-resolves even when sp.lock is
    // up-to-date (used by `spm update`). frozen fails instead of
    // re-resolving or re-locking when sp.lock is out-of-sync (used by
    // `spm install --frozen`). Registry packages are fetched
    // from the registry (SHA-256 verified) and extracted into
    // <projectDir>/.spm/packages/<name>/; a registry that cannot be
    // reached falls back to the local cache with a printed note, and
    // fails loudly when neither has the bytes. Legacy lockfiles whose
    // registry checksums are metadata hashes (pre-Phase-4) are re-locked
    // with real tarball SHA-256 values without changing versions.
    static InstallResult install(const std::string& projectDir,
                                 const std::string& registryUrl = "",
                                 bool forceResolve = false,
                                 bool frozen = false);

    // Test seam: same as install() but with an injected registry client
    // (unit tests substitute a stub instead of a live server).
    static InstallResult install(const std::string& projectDir,
                                 std::shared_ptr<class IRegistryClient> client,
                                 bool forceResolve = false,
                                 bool frozen = false);

    // Ensure every registry package in `lock` is vendored into
    // <projectDir>/.spm/packages/<name>/ (download + SHA-256 verify +
    // extract when missing or stale). Packages already vendored with a
    // matching checksum marker are skipped. Returns false + error on
    // any missing/unverifiable package — never a partial silent install.
    static bool ensureVendored(const std::string& projectDir, const Lockfile& lock,
                               const std::string& registryUrl, std::string& errorMessage);

    // Test seam: same as ensureVendored() but with an injected client.
    static bool ensureVendored(const std::string& projectDir, const Lockfile& lock,
                               std::shared_ptr<class IRegistryClient> client,
                               std::string& errorMessage);

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

    // Search registry for packages (Phase 4 slice A: queries the real
    // registry first; falls back to the built-in offline index with an
    // explicit warning when the registry is unreachable). An empty
    // registryUrl resolves via defaultRegistryUrl().
    static std::vector<SearchResult> search(const std::string& query, const std::string& registryUrl = "");

private:
    static std::string readFile(const std::string& path);
};

} // namespace sp::spm
