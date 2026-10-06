#pragma once

#include "ProjectConfig.h"
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
};

// Default provider for local filesystem paths and mock registry
class DefaultPackageProvider : public IPackageProvider {
public:
    DefaultPackageProvider() = default;

    void registerMockPackage(const ProjectConfig& config);

    std::vector<Version> getAvailableVersions(const std::string& name) override;
    std::optional<ProjectConfig> getPackageConfig(const std::string& name, const Version& version) override;
    std::optional<ProjectConfig> resolvePathDependency(const std::string& relativeOrAbsolutePath,
                                                       const std::string& baseDir,
                                                       std::string& outAbsPath) override;

private:
    std::unordered_map<std::string, std::vector<ProjectConfig>> m_mockRegistry;
};

class DependencyResolver {
public:
    explicit DependencyResolver(std::shared_ptr<IPackageProvider> provider = nullptr);

    ResolutionResult resolve(const ProjectConfig& rootConfig,
                             const std::string& rootProjectDir = ".",
                             bool includeDev = false);

private:
    std::shared_ptr<IPackageProvider> m_provider;

    static std::string computeChecksum(const std::string& name, const Version& v, const std::string& source);
};

} // namespace sp::spm
