#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace sp::spm {

struct Dependency {
    std::string name;
    std::string versionReq = "*"; // e.g. "^1.0.0", ">=1.2.0", "*"
    std::string path;             // optional local path: { path = "../foo" }
    std::string gitUrl;           // optional git repo: { git = "https://..." }
};

struct PackageMetadata {
    std::string name;
    std::string version = "0.1.0";
    std::string description;
    std::string author;
    std::string edition = "2026";
    std::string entry = "src/main.sp";
    std::string packageType = "bin"; // "bin" or "lib"
};

class ProjectConfig {
public:
    ProjectConfig() = default;

    static ProjectConfig createDefault(const std::string& name, bool isLib = false);

    bool loadFromFile(const std::string& filePath);
    bool loadFromString(const std::string& tomlContent);
    bool saveToFile(const std::string& filePath) const;
    std::string toTomlString() const;

    const PackageMetadata& getPackage() const { return m_package; }
    PackageMetadata& getPackage() { return m_package; }
    void setPackage(const PackageMetadata& pkg) { m_package = pkg; }

    const std::vector<Dependency>& getDependencies() const { return m_dependencies; }
    const std::vector<Dependency>& getDevDependencies() const { return m_devDependencies; }

    bool addDependency(const Dependency& dep, bool isDev = false);
    bool removeDependency(const std::string& name, bool isDev = false);
    const Dependency* findDependency(const std::string& name, bool isDev = false) const;
    bool hasDependency(const std::string& name) const;

    const std::string& getLastError() const { return m_lastError; }

private:
    PackageMetadata m_package;
    std::vector<Dependency> m_dependencies;
    std::vector<Dependency> m_devDependencies;
    mutable std::string m_lastError;

    static std::string trim(const std::string& s);
    static std::string stripQuotes(const std::string& s);
    static Dependency parseDependencyValue(const std::string& name, const std::string& val);
};

// Project scaffolding helper
bool scaffoldProject(const std::string& projectDir, const std::string& name, bool isLib, std::string& errorMsg);

} // namespace sp::spm
