#pragma once

#include "SemVer.h"
#include "DependencyResolver.h"
#include "ProjectConfig.h"
#include <string>
#include <vector>
#include <optional>

namespace sp::spm {

struct LockedPackage {
    std::string name;
    std::string version;
    std::string source;
    std::string checksum;
    std::vector<std::string> dependencies;
};

class Lockfile {
public:
    Lockfile() = default;

    static Lockfile fromResolution(const ResolutionResult& res);

    bool loadFromFile(const std::string& filePath);
    bool loadFromString(const std::string& content);
    bool saveToFile(const std::string& filePath) const;
    std::string toTomlString() const;

    const std::vector<LockedPackage>& getPackages() const { return m_packages; }
    void addPackage(const LockedPackage& pkg) { m_packages.push_back(pkg); }
    const LockedPackage* findPackage(const std::string& name) const;

    int getVersion() const { return m_version; }
    void setVersion(int v) { m_version = v; }

    const std::string& getLastError() const { return m_lastError; }

    // Validate if lockfile satisfies all direct dependencies of project config
    bool isUpToDate(const ProjectConfig& config) const;

private:
    int m_version{1};
    std::vector<LockedPackage> m_packages;
    mutable std::string m_lastError;

    static std::string trim(const std::string& s);
    static std::string stripQuotes(const std::string& s);
};

} // namespace sp::spm
