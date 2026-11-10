#pragma once

#include "ProjectConfig.h"
#include "RegistryClient.h"
#include "SemVer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace sp::spm {

enum class PackageSourceKind {
    Path,
    Git,
    Registry
};

std::string sourceKindToString(PackageSourceKind kind);

struct ResolvedPackage {
    std::string name;
    Version version;
    PackageSourceKind sourceKind{PackageSourceKind::Registry};
    std::string sourceLocation; // path, git URL, or registry name
    std::string checksum;
    std::vector<std::string> dependencies; // names of direct dependencies
    std::string projectDir; // directory where sp.toml and src reside
};

struct ResolutionResult {
    bool success{false};
    std::string errorMessage;
    std::vector<ResolvedPackage> resolvedPackages; // topologically sorted: dependencies before dependents

    const ResolvedPackage* findPackage(const std::string& name) const {
        for (const auto& pkg : resolvedPackages) {
            if (pkg.name == name) return &pkg;
        }
        return nullptr;
    }
};

// Interface for retrieving package manifests
class IPackageProvider {
public:
    virtual ~IPackageProvider() = default;
    virtual std::vector<Version> getAvailableVersions(const std::string& name) = 0;
    virtual std::optional<ProjectConfig> getPackageConfig(const std::string& name, const Version& version) = 0;
    virtual std::optional<ProjectConfig> resolvePathDependency(const std::string& relativeOrAbsolutePath,
                                                               const std::string& baseDir,
                                                               std::string& outAbsPath) = 0;
    // Content checksum for a package version, when the provider knows one
    // (the registry's published tarball SHA-256, or — when the registry
    // is unreachable — the cached archive's own SHA-256). The resolver
    // stores it in ResolvedPackage.checksum / sp.lock instead of the
    // metadata hash. Providers without content knowledge return nullopt
    // and the resolver keeps the legacy metadata hash
    // (DependencyResolver::computeChecksum).
    virtual std::optional<std::string> getPackageChecksum(const std::string& name,
                                                          const Version& version) {
        (void)name;
        (void)version;
        return std::nullopt;
    }

    // Reason for the most recent getPackageConfig failure, if any.
    // Lets the resolver tell "no config because the download failed"
    // apart from "no such version" instead of silently picking an older
    // version when the newest match cannot be fetched.
    virtual std::string lastError() const { return ""; }
};

// Default provider for local filesystem paths and mock registry
class DefaultPackageProvider : public IPackageProvider {
public:
    DefaultPackageProvider();

    void registerMockPackage(const ProjectConfig& config);

    std::vector<Version> getAvailableVersions(const std::string& name) override;
    std::optional<ProjectConfig> getPackageConfig(const std::string& name, const Version& version) override;
    std::optional<ProjectConfig> resolvePathDependency(const std::string& relativeOrAbsolutePath,
                                                       const std::string& baseDir,
                                                       std::string& outAbsPath) override;

private:
    std::unordered_map<std::string, std::vector<ProjectConfig>> m_mockRegistry;
};

// Registry-first provider — Phase 4 slice A.
//
// Resolution order for a name:
//   1. The real registry (IRegistryClient): versions + tarball SHA-256.
//      On transport failure the registry is treated as unavailable and
//      resolution falls back to (2) with a recorded note — an offline
//      install still works from cache, but never pretends the registry
//      was consulted.
//   2. DefaultPackageProvider (mock registry + local package cache).
//
// getPackageConfig for a registry version downloads the tarball (SHA-256
// verified by the client) into the local cache first, because the
// registry's metadata carries no dependency list — the transitive deps
// live in the package's sp.toml, which is inside the archive. A version
// the registry does not know is delegated untouched.
class RegistryProvider : public IPackageProvider {
public:
    explicit RegistryProvider(std::shared_ptr<IRegistryClient> client);

    std::vector<Version> getAvailableVersions(const std::string& name) override;
    std::optional<ProjectConfig> getPackageConfig(const std::string& name, const Version& version) override;
    std::optional<ProjectConfig> resolvePathDependency(const std::string& relativeOrAbsolutePath,
                                                       const std::string& baseDir,
                                                       std::string& outAbsPath) override;
    std::optional<std::string> getPackageChecksum(const std::string& name,
                                                  const Version& version) override;

    // True when the last registry contact failed (transport error).
    // Callers print one fallback note per operation from lastRegistryError().
    bool registryUnavailable() const { return m_registryUnavailable; }
    const std::string& lastRegistryError() const { return m_lastRegistryError; }
    std::string lastError() const override { return m_lastConfigError; }

private:
    // Registry index, fetched at most once per provider instance.
    // Returns false when the registry could not be reached at all.
    bool ensureIndex(std::string& err);
    // Metadata for one version, from the cached index when present.
    std::optional<RegistryPackageInfo> findInIndex(const std::string& name,
                                                   const std::string& version) const;

    std::shared_ptr<IRegistryClient> m_client;
    DefaultPackageProvider m_fallback;
    bool m_indexLoaded{false};
    bool m_registryUnavailable{false};
    std::string m_lastRegistryError;
    std::string m_lastConfigError;
    std::vector<RegistryPackageInfo> m_index;
};

class DependencyResolver {
public:
    explicit DependencyResolver(std::shared_ptr<IPackageProvider> provider = nullptr);

    ResolutionResult resolve(const ProjectConfig& rootConfig,
                             const std::string& rootProjectDir = ".",
                             bool includeDev = false);

    static std::string computeChecksum(const std::string& name, const Version& v, const std::string& source);

private:
    std::shared_ptr<IPackageProvider> m_provider;
};

} // namespace sp::spm
