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
    static ProjectConfig createWorkspace(const std::vector<std::string>& members);

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

    // Workspace monorepo support
    bool isWorkspace() const { return m_isWorkspace; }
    void setWorkspace(bool ws) { m_isWorkspace = ws; }
    const std::vector<std::string>& getWorkspaceMembers() const { return m_workspaceMembers; }
    void addWorkspaceMember(const std::string& member) {
        m_isWorkspace = true;
        m_workspaceMembers.push_back(member);
    }

    // Phase 39 slice B: [native] section — native libraries to link
    // (bare names, e.g. "mini" -> -lmini) and extra library search
    // paths. Used by the spm build path (BuildOrchestrator).
    const std::vector<std::string>& getNativeLibs() const { return m_nativeLibs; }
    const std::vector<std::string>& getNativeLibPaths() const { return m_nativeLibPaths; }
    void setNativeLibs(std::vector<std::string> libs) { m_nativeLibs = std::move(libs); }
    void setNativeLibPaths(std::vector<std::string> paths) { m_nativeLibPaths = std::move(paths); }

    const std::string& getLastError() const { return m_lastError; }

private:
    PackageMetadata m_package;
    std::vector<Dependency> m_dependencies;
    std::vector<Dependency> m_devDependencies;
    bool m_isWorkspace{false};
    std::vector<std::string> m_workspaceMembers;
    std::vector<std::string> m_nativeLibs;
    std::vector<std::string> m_nativeLibPaths;
    mutable std::string m_lastError;

    static std::string trim(const std::string& s);
    static std::string stripQuotes(const std::string& s);
    // Phase 39 slice B: split a TOML string-array body into values.
    static void parseStringArrayItems(const std::string& items, std::vector<std::string>& out);
    static Dependency parseDependencyValue(const std::string& name, const std::string& val);
};

// Project scaffolding helper
bool scaffoldProject(const std::string& projectDir, const std::string& name, bool isLib, std::string& errorMsg, const std::string& templateType = "");

} // namespace sp::spm
