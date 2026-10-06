#include "DependencyResolver.h"
#include <filesystem>
#include <functional>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <set>

namespace fs = std::filesystem;

namespace sp::spm {

std::string sourceKindToString(PackageSourceKind kind) {
    switch (kind) {
        case PackageSourceKind::Path: return "path";
        case PackageSourceKind::Git: return "git";
        case PackageSourceKind::Registry: return "registry";
    }
    return "registry";
}

DefaultPackageProvider::DefaultPackageProvider() {
    auto addStd = [this](const std::string& name, const std::string& ver, const std::string& desc) {
        ProjectConfig cfg;
        cfg.getPackage().name = name;
        cfg.getPackage().version = ver;
        cfg.getPackage().description = desc;
        cfg.getPackage().packageType = "lib";
        registerMockPackage(cfg);
    };

    addStd("sp_math", "1.0.0", "Advanced mathematics, linear algebra, and statistical functions for SP");
    addStd("sp_json", "0.8.2", "High-performance JSON parser and serializer with zero-copy decoding");
    addStd("sp_http", "0.4.0", "Fast HTTP client and server library with async IO support");
    addStd("sp_logger", "1.2.0", "Structured terminal and file logging utility for SP applications");
    addStd("sp_test", "1.0.1", "BDD-style unit and benchmark testing framework for SP");
    addStd("sp_crypto", "0.3.0", "Cryptographic primitives, SHA-256, AES, and secure hashing");
    addStd("sp_csv", "0.2.1", "Fast CSV reader, writer, and typed data record parser");
}

void DefaultPackageProvider::registerMockPackage(const ProjectConfig& config) {
    m_mockRegistry[config.getPackage().name].push_back(config);
}

std::vector<Version> DefaultPackageProvider::getAvailableVersions(const std::string& name) {
    std::vector<Version> versions;
    auto it = m_mockRegistry.find(name);
    if (it != m_mockRegistry.end()) {
        for (const auto& cfg : it->second) {
            auto v = Version::parse(cfg.getPackage().version);
            if (v) versions.push_back(*v);
        }
    }
    std::sort(versions.rbegin(), versions.rend()); // descending order
    return versions;
}

std::optional<ProjectConfig> DefaultPackageProvider::getPackageConfig(const std::string& name, const Version& version) {
    auto it = m_mockRegistry.find(name);
    if (it != m_mockRegistry.end()) {
        for (const auto& cfg : it->second) {
            auto v = Version::parse(cfg.getPackage().version);
            if (v && *v == version) {
                return cfg;
            }
        }
    }
    return std::nullopt;
}

std::optional<ProjectConfig> DefaultPackageProvider::resolvePathDependency(
    const std::string& relativeOrAbsolutePath,
    const std::string& baseDir,
    std::string& outAbsPath)
{
    fs::path p = relativeOrAbsolutePath;
    if (p.is_relative()) {
        p = fs::path(baseDir) / p;
    }

    std::error_code ec;
    fs::path canonicalPath = fs::weakly_canonical(p, ec);
    if (ec) {
        canonicalPath = fs::absolute(p);
    }
    outAbsPath = canonicalPath.string();

    fs::path tomlPath = canonicalPath / "sp.toml";
    if (!fs::exists(tomlPath)) {
        return std::nullopt;
    }

    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        return std::nullopt;
    }
    return config;
}

DependencyResolver::DependencyResolver(std::shared_ptr<IPackageProvider> provider)
    : m_provider(std::move(provider))
{
    if (!m_provider) {
        m_provider = std::make_shared<DefaultPackageProvider>();
    }
}

std::string DependencyResolver::computeChecksum(const std::string& name, const Version& v, const std::string& source) {
    // Deterministic hash representation
    std::string combined = name + "@" + v.toString() + "(" + source + ")";
    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0x100000001b3ULL;
    for (char c : combined) {
        h1 = (h1 ^ static_cast<uint64_t>(c)) * 0x100000001b3ULL;
        h2 = (h2 ^ static_cast<uint64_t>(c)) * 0x517cc1b727220a95ULL;
    }
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << h1
        << std::setw(16) << h2
        << std::setw(16) << (h1 ^ h2)
        << std::setw(16) << (h1 + h2);
    return oss.str();
}

namespace {

struct PackageConstraintRecord {
    std::string requiredBy;
    std::string reqStr;
    VersionReq req;
};

struct ResolutionState {
    std::unordered_map<std::string, ResolvedPackage> resolved;
    std::unordered_map<std::string, std::vector<PackageConstraintRecord>> constraints;
    std::unordered_map<std::string, ProjectConfig> configs;
    std::vector<std::string> activeStack;
};

} // namespace

ResolutionResult DependencyResolver::resolve(
    const ProjectConfig& rootConfig,
    const std::string& rootProjectDir,
    bool includeDev)
{
    ResolutionResult result;
    ResolutionState state;

    std::vector<Dependency> initialDeps = rootConfig.getDependencies();
    if (includeDev) {
        for (const auto& devDep : rootConfig.getDevDependencies()) {
            initialDeps.push_back(devDep);
        }
    }

    // Helper recursive resolution function
    std::function<bool(const std::string&, const Dependency&, const std::string&)> resolveDep;

    resolveDep = [&](const std::string& parentName, const Dependency& dep, const std::string& baseDir) -> bool {
        const std::string& pkgName = dep.name;

        // 1. Cycle detection
        auto stackIt = std::find(state.activeStack.begin(), state.activeStack.end(), pkgName);
        if (stackIt != state.activeStack.end()) {
            std::ostringstream oss;
            oss << "Circular dependency detected: ";
            for (auto it = stackIt; it != state.activeStack.end(); ++it) {
                oss << *it << " -> ";
            }
            oss << pkgName;
            result.errorMessage = oss.str();
            return false;
        }

        // 2. Parse requirement
        auto parsedReq = VersionReq::parse(dep.versionReq);
        if (!parsedReq) {
            result.errorMessage = "Invalid version requirement '" + dep.versionReq + "' for package '" + pkgName + "'";
            return false;
        }

        state.constraints[pkgName].push_back(PackageConstraintRecord{
            parentName,
            dep.versionReq,
            *parsedReq
        });

        // 3. Check if already resolved
        auto existingIt = state.resolved.find(pkgName);
        if (existingIt != state.resolved.end()) {
            // Check if existing resolved version satisfies the new constraint
            if (!parsedReq->matches(existingIt->second.version)) {
                const auto& firstReq = state.constraints[pkgName].front();
                result.errorMessage = "Version conflict for package '" + pkgName + "': required '" +
                    firstReq.reqStr + "' by '" + firstReq.requiredBy + "', but '" +
                    dep.versionReq + "' required by '" + parentName + "'";
                return false;
            }
            // Already resolved and compatible!
            return true;
        }

        // 4. Fetch package config
        ProjectConfig pkgConfig;
        PackageSourceKind sourceKind = PackageSourceKind::Registry;
        std::string sourceLocation;
        std::string projectDir;
        Version chosenVersion;

        if (!dep.path.empty()) {
            sourceKind = PackageSourceKind::Path;
            std::string absPath;
            auto optCfg = m_provider->resolvePathDependency(dep.path, baseDir, absPath);
            if (!optCfg) {
                result.errorMessage = "Failed to resolve path dependency '" + pkgName + "' at '" + dep.path + "'";
                return false;
            }
            pkgConfig = *optCfg;
            sourceLocation = dep.path;
            projectDir = absPath;

            if (pkgConfig.getPackage().name != pkgName) {
                result.errorMessage = "Package name mismatch in '" + dep.path + "': expected '" +
                    pkgName + "', found '" + pkgConfig.getPackage().name + "'";
                return false;
            }

            auto optV = Version::parse(pkgConfig.getPackage().version);
            if (!optV) {
                result.errorMessage = "Invalid version '" + pkgConfig.getPackage().version + "' in package '" + pkgName + "'";
                return false;
            }
            chosenVersion = *optV;

            if (!parsedReq->matches(chosenVersion)) {
                result.errorMessage = "Path dependency '" + pkgName + "' version " + chosenVersion.toString() +
                    " does not satisfy requirement '" + dep.versionReq + "'";
                return false;
            }
        } else if (!dep.gitUrl.empty()) {
            sourceKind = PackageSourceKind::Git;
            sourceLocation = dep.gitUrl;
            // Git repo without path provider falls back to mock registry / version requirement
            auto avail = m_provider->getAvailableVersions(pkgName);
            bool found = false;
            for (const auto& v : avail) {
                if (parsedReq->matches(v)) {
                    auto optCfg = m_provider->getPackageConfig(pkgName, v);
                    if (optCfg) {
                        pkgConfig = *optCfg;
                        chosenVersion = v;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                result.errorMessage = "No available git package found matching '" + dep.gitUrl + "' for '" + pkgName + "'";
                return false;
            }
        } else {
            // Registry dependency
            sourceKind = PackageSourceKind::Registry;
            sourceLocation = "registry";
            auto avail = m_provider->getAvailableVersions(pkgName);
            bool found = false;
            for (const auto& v : avail) {
                // Must satisfy all accumulated constraints
                bool allSatisfied = true;
                for (const auto& c : state.constraints[pkgName]) {
                    if (!c.req.matches(v)) {
                        allSatisfied = false;
                        break;
                    }
                }
                if (allSatisfied) {
                    auto optCfg = m_provider->getPackageConfig(pkgName, v);
                    if (optCfg) {
                        pkgConfig = *optCfg;
                        chosenVersion = v;
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                const auto& firstReq = state.constraints[pkgName].front();
                result.errorMessage = "No compatible version found for package '" + pkgName +
                    "' matching requirement '" + firstReq.reqStr + "'";
                return false;
            }
        }

        // 5. Register in resolved
        ResolvedPackage resolvedPkg;
        resolvedPkg.name = pkgName;
        resolvedPkg.version = chosenVersion;
        resolvedPkg.sourceKind = sourceKind;
        resolvedPkg.sourceLocation = sourceLocation;
        resolvedPkg.projectDir = projectDir;
        resolvedPkg.checksum = computeChecksum(pkgName, chosenVersion, sourceLocation);

        for (const auto& childDep : pkgConfig.getDependencies()) {
            resolvedPkg.dependencies.push_back(childDep.name);
        }

        state.resolved[pkgName] = resolvedPkg;
        state.configs[pkgName] = pkgConfig;

        // 6. Recursively resolve sub-dependencies
        state.activeStack.push_back(pkgName);
        for (const auto& childDep : pkgConfig.getDependencies()) {
            std::string subBase = projectDir.empty() ? baseDir : projectDir;
            if (!resolveDep(pkgName, childDep, subBase)) {
                return false;
            }
        }
        state.activeStack.pop_back();

        return true;
    };

    // Root node
    const std::string rootName = rootConfig.getPackage().name.empty() ? "<root>" : rootConfig.getPackage().name;
    state.activeStack.push_back(rootName);

    for (const auto& dep : initialDeps) {
        if (!resolveDep(rootName, dep, rootProjectDir)) {
            result.success = false;
            return result;
        }
    }

    state.activeStack.pop_back();

    // 7. Topological Sort (Kahn's algorithm)
    // Leaf dependencies come first
    std::unordered_map<std::string, int> inDegree;
    std::unordered_map<std::string, std::vector<std::string>> graph; // dep -> packages that depend on it

    for (const auto& [name, _] : state.resolved) {
        inDegree[name] = 0;
    }

    for (const auto& [name, pkg] : state.resolved) {
        for (const auto& depName : pkg.dependencies) {
            if (state.resolved.count(depName)) {
                graph[depName].push_back(name);
                inDegree[name]++;
            }
        }
    }

    std::queue<std::string> q;
    for (const auto& [name, deg] : inDegree) {
        if (deg == 0) {
            q.push(name);
        }
    }

    std::vector<ResolvedPackage> sorted;
    while (!q.empty()) {
        std::string u = q.front();
        q.pop();

        sorted.push_back(state.resolved[u]);

        for (const auto& v : graph[u]) {
            inDegree[v]--;
            if (inDegree[v] == 0) {
                q.push(v);
            }
        }
    }

    if (sorted.size() != state.resolved.size()) {
        // Cycle amongst resolved dependencies that was not captured earlier
        result.errorMessage = "Cycle detected in dependency graph during topological sort";
        result.success = false;
        return result;
    }

    result.resolvedPackages = std::move(sorted);
    result.success = true;
    return result;
}

} // namespace sp::spm
