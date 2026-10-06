#include "ProjectConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace sp::spm {

std::string ProjectConfig::trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }
    if (start == s.size()) return "";
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }
    return s.substr(start, end - start);
}

std::string ProjectConfig::stripQuotes(const std::string& s) {
    std::string str = trim(s);
    if (str.size() >= 2) {
        if ((str.front() == '"' && str.back() == '"') ||
            (str.front() == '\'' && str.back() == '\'')) {
            return str.substr(1, str.size() - 2);
        }
    }
    return str;
}

Dependency ProjectConfig::parseDependencyValue(const std::string& name, const std::string& val) {
    Dependency dep;
    dep.name = name;
    std::string v = trim(val);

    if (v.empty()) {
        dep.versionReq = "*";
        return dep;
    }

    if (v.front() == '{' && v.back() == '}') {
        // Inline table: { path = "../foo", version = "1.0.0", git = "..." }
        std::string inner = v.substr(1, v.size() - 2);
        std::stringstream ss(inner);
        std::string item;
        while (std::getline(ss, item, ',')) {
            size_t eq = item.find('=');
            if (eq != std::string::npos) {
                std::string k = trim(item.substr(0, eq));
                std::string valStr = stripQuotes(item.substr(eq + 1));
                if (k == "path") {
                    dep.path = valStr;
                } else if (k == "version") {
                    dep.versionReq = valStr;
                } else if (k == "git") {
                    dep.gitUrl = valStr;
                }
            }
        }
        if (dep.versionReq.empty()) {
            dep.versionReq = "*";
        }
    } else {
        // Plain version requirement string
        dep.versionReq = stripQuotes(v);
        if (dep.versionReq.empty()) {
            dep.versionReq = "*";
        }
    }

    return dep;
}

ProjectConfig ProjectConfig::createDefault(const std::string& name, bool isLib) {
    ProjectConfig config;
    config.m_package.name = name.empty() ? "unnamed_project" : name;
    config.m_package.version = "0.1.0";
    config.m_package.description = "A new SP " + std::string(isLib ? "library" : "package");
    config.m_package.author = "";
    config.m_package.edition = "2026";
    config.m_package.packageType = isLib ? "lib" : "bin";
    config.m_package.entry = isLib ? "src/lib.sp" : "src/main.sp";
    return config;
}

bool ProjectConfig::loadFromString(const std::string& tomlContent) {
    m_dependencies.clear();
    m_devDependencies.clear();
    m_package = PackageMetadata();

    std::stringstream ss(tomlContent);
    std::string line;
    std::string currentSection;

    while (std::getline(ss, line)) {
        // Strip comments
        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos) {
            line = line.substr(0, hashPos);
        }
        line = trim(line);
        if (line.empty()) continue;

        // Section header
        if (line.front() == '[' && line.back() == ']') {
            currentSection = trim(line.substr(1, line.size() - 2));
            continue;
        }

        // Key = Value
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trim(line.substr(0, eqPos));
        std::string value = trim(line.substr(eqPos + 1));

        if (currentSection == "package") {
            if (key == "name") {
                m_package.name = stripQuotes(value);
            } else if (key == "version") {
                m_package.version = stripQuotes(value);
            } else if (key == "description") {
                m_package.description = stripQuotes(value);
            } else if (key == "author" || key == "authors") {
                m_package.author = stripQuotes(value);
            } else if (key == "edition") {
                m_package.edition = stripQuotes(value);
            } else if (key == "entry") {
                m_package.entry = stripQuotes(value);
            } else if (key == "type") {
                m_package.packageType = stripQuotes(value);
            }
        } else if (currentSection == "dependencies") {
            Dependency dep = parseDependencyValue(key, value);
            m_dependencies.push_back(dep);
        } else if (currentSection == "dev-dependencies") {
            Dependency dep = parseDependencyValue(key, value);
            m_devDependencies.push_back(dep);
        }
    }

    if (m_package.name.empty()) {
        m_lastError = "Missing [package] name in sp.toml";
        return false;
    }

    return true;
}

bool ProjectConfig::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_lastError = "Could not open configuration file: " + filePath;
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return loadFromString(ss.str());
}

std::string ProjectConfig::toTomlString() const {
    std::ostringstream ss;

    ss << "[package]\n";
    ss << "name = \"" << m_package.name << "\"\n";
    ss << "version = \"" << m_package.version << "\"\n";
    if (!m_package.description.empty()) {
        ss << "description = \"" << m_package.description << "\"\n";
    }
    if (!m_package.author.empty()) {
        ss << "author = \"" << m_package.author << "\"\n";
    }
    ss << "edition = \"" << m_package.edition << "\"\n";
    ss << "entry = \"" << m_package.entry << "\"\n";
    ss << "type = \"" << m_package.packageType << "\"\n";

    ss << "\n[dependencies]\n";
    for (const auto& dep : m_dependencies) {
        if (!dep.path.empty()) {
            ss << dep.name << " = { path = \"" << dep.path << "\"";
            if (!dep.versionReq.empty() && dep.versionReq != "*") {
                ss << ", version = \"" << dep.versionReq << "\"";
            }
            ss << " }\n";
        } else if (!dep.gitUrl.empty()) {
            ss << dep.name << " = { git = \"" << dep.gitUrl << "\" }\n";
        } else {
            ss << dep.name << " = \"" << dep.versionReq << "\"\n";
        }
    }

    if (!m_devDependencies.empty()) {
        ss << "\n[dev-dependencies]\n";
        for (const auto& dep : m_devDependencies) {
            if (!dep.path.empty()) {
                ss << dep.name << " = { path = \"" << dep.path << "\"";
                if (!dep.versionReq.empty() && dep.versionReq != "*") {
                    ss << ", version = \"" << dep.versionReq << "\"";
                }
                ss << " }\n";
            } else if (!dep.gitUrl.empty()) {
                ss << dep.name << " = { git = \"" << dep.gitUrl << "\" }\n";
            } else {
                ss << dep.name << " = \"" << dep.versionReq << "\"\n";
            }
        }
    }

    return ss.str();
}

bool ProjectConfig::saveToFile(const std::string& filePath) const {
    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        m_lastError = "Could not open file for writing: " + filePath;
        return false;
    }

    file << toTomlString();
    return file.good();
}

bool ProjectConfig::addDependency(const Dependency& dep, bool isDev) {
    auto& list = isDev ? m_devDependencies : m_dependencies;
    for (auto& existing : list) {
        if (existing.name == dep.name) {
            existing = dep;
            return true;
        }
    }
    list.push_back(dep);
    return true;
}

bool ProjectConfig::removeDependency(const std::string& name, bool isDev) {
    auto& list = isDev ? m_devDependencies : m_dependencies;
    auto it = std::remove_if(list.begin(), list.end(), [&](const Dependency& d) {
        return d.name == name;
    });
    if (it != list.end()) {
        list.erase(it, list.end());
        return true;
    }
    return false;
}

const Dependency* ProjectConfig::findDependency(const std::string& name, bool isDev) const {
    const auto& list = isDev ? m_devDependencies : m_dependencies;
    for (const auto& d : list) {
        if (d.name == name) return &d;
    }
    return nullptr;
}

bool ProjectConfig::hasDependency(const std::string& name) const {
    return findDependency(name, false) != nullptr || findDependency(name, true) != nullptr;
}

bool scaffoldProject(const std::string& projectDir, const std::string& name, bool isLib, std::string& errorMsg) {
    fs::path pDir = fs::absolute(projectDir);
    std::error_code ec;

    if (!fs::exists(pDir)) {
        if (!fs::create_directories(pDir, ec)) {
            errorMsg = "Failed to create directory: " + pDir.string();
            return false;
        }
    }

    fs::path tomlPath = pDir / "sp.toml";
    if (fs::exists(tomlPath)) {
        errorMsg = "Project already exists in " + pDir.string() + " (sp.toml exists)";
        return false;
    }

    std::string projName = name;
    if (projName.empty()) {
        projName = pDir.filename().string();
    }
    if (projName.empty()) {
        projName = "my_project";
    }

    ProjectConfig config = ProjectConfig::createDefault(projName, isLib);
    if (!config.saveToFile(tomlPath.string())) {
        errorMsg = config.getLastError();
        return false;
    }

    // Create src directory
    fs::path srcDir = pDir / "src";
    fs::create_directories(srcDir, ec);

    // Create entry file
    if (!isLib) {
        fs::path mainPath = srcDir / "main.sp";
        if (!fs::exists(mainPath)) {
            std::ofstream mainFile(mainPath);
            mainFile << "import std.io;\n\n"
                     << "fn main() {\n"
                     << "    println(\"Hello, SP World from " << projName << "!\");\n"
                     << "}\n";
        }
    } else {
        fs::path libPath = srcDir / "lib.sp";
        if (!fs::exists(libPath)) {
            std::ofstream libFile(libPath);
            libFile << "module " << projName << ";\n\n"
                    << "public fn greet(string target) -> string {\n"
                    << "    return \"Hello, \" + target + \" from " << projName << "!\";\n"
                    << "}\n";
        }
    }

    // Create .gitignore
    fs::path gitignorePath = pDir / ".gitignore";
    if (!fs::exists(gitignorePath)) {
        std::ofstream gitignore(gitignorePath);
        gitignore << "target/\n"
                  << "*.tmp.ll\n"
                  << "*.exe\n"
                  << "*.o\n"
                  << "*.obj\n";
    }

    return true;
}

} // namespace sp::spm
