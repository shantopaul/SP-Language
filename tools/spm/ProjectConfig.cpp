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

ProjectConfig ProjectConfig::createWorkspace(const std::vector<std::string>& members) {
    ProjectConfig config;
    config.m_isWorkspace = true;
    config.m_workspaceMembers = members;
    return config;
}

bool ProjectConfig::loadFromString(const std::string& tomlContent) {
    m_dependencies.clear();
    m_devDependencies.clear();
    m_package = PackageMetadata();
    m_isWorkspace = false;
    m_workspaceMembers.clear();

    std::stringstream ss(tomlContent);
    std::string line;
    std::string currentSection;
    bool inWorkspaceMembersArray = false;

    while (std::getline(ss, line)) {
        // Strip comments
        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos) {
            line = line.substr(0, hashPos);
        }
        line = trim(line);
        if (line.empty()) continue;

        // If accumulating multiline workspace members
        if (inWorkspaceMembersArray) {
            size_t closePos = line.find(']');
            std::string items = (closePos != std::string::npos) ? line.substr(0, closePos) : line;
            std::stringstream mss(items);
            std::string item;
            while (std::getline(mss, item, ',')) {
                std::string mem = stripQuotes(trim(item));
                if (!mem.empty()) {
                    m_workspaceMembers.push_back(mem);
                }
            }
            if (closePos != std::string::npos) {
                inWorkspaceMembersArray = false;
            }
            continue;
        }

        // Section header
        if (line.front() == '[' && line.back() == ']') {
            currentSection = trim(line.substr(1, line.size() - 2));
            if (currentSection == "workspace") {
                m_isWorkspace = true;
            }
            continue;
        }

        // Key = Value
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trim(line.substr(0, eqPos));
        std::string value = trim(line.substr(eqPos + 1));

        if (currentSection == "workspace") {
            m_isWorkspace = true;
            if (key == "members") {
                size_t openPos = value.find('[');
                size_t closePos = value.find(']');
                if (openPos != std::string::npos && closePos != std::string::npos) {
                    std::string items = value.substr(openPos + 1, closePos - openPos - 1);
                    std::stringstream mss(items);
                    std::string item;
                    while (std::getline(mss, item, ',')) {
                        std::string mem = stripQuotes(trim(item));
                        if (!mem.empty()) {
                            m_workspaceMembers.push_back(mem);
                        }
                    }
                } else if (openPos != std::string::npos) {
                    inWorkspaceMembersArray = true;
                    std::string items = value.substr(openPos + 1);
                    std::stringstream mss(items);
                    std::string item;
                    while (std::getline(mss, item, ',')) {
                        std::string mem = stripQuotes(trim(item));
                        if (!mem.empty()) {
                            m_workspaceMembers.push_back(mem);
                        }
                    }
                }
            }
        } else if (currentSection == "package") {
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
        if (m_isWorkspace) {
            // Valid workspace root without a package
            return true;
        }
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

    if (m_isWorkspace) {
        ss << "[workspace]\nmembers = [\n";
        for (size_t i = 0; i < m_workspaceMembers.size(); ++i) {
            ss << "    \"" << m_workspaceMembers[i] << "\"" << (i + 1 < m_workspaceMembers.size() ? ",\n" : "\n");
        }
        ss << "]\n\n";
    }

    if (!m_package.name.empty()) {
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
    }

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

bool scaffoldProject(const std::string& projectDir, const std::string& name, bool isLib, std::string& errorMsg, const std::string& templateType) {
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
    if (templateType == "api") {
        config.getPackage().description = "High-performance SP REST API service";
        config.addDependency({"std.http", ">=2.3.0", "", ""});
        config.addDependency({"std.web", ">=2.7.0", "", ""});
        config.addDependency({"std.json", ">=2.4.0", "", ""});
    }
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
            if (templateType == "api") {
                mainFile << "module main;\n\n"
                         << "import std.io;\n"
                         << "import std.web;\n\n"
                         << "fn main() {\n"
                         << "    let app = new App();\n\n"
                         << "    // Production middleware pipeline\n"
                         << "    app.use(new Logger());\n"
                         << "    app.use(new Cors());\n"
                         << "    app.use(new Recovery());\n\n"
                         << "    // Interactive API Documentation (OpenAPI 3.0.3)\n"
                         << "    app.swagger(\"/docs\", \"/openapi.json\", \"" << projName << " API\");\n"
                         << "    app.redoc(\"/redoc\", \"/openapi.json\", \"" << projName << " API\");\n\n"
                         << "    // REST endpoints\n"
                         << "    app.get(\"/api/health\", (ptr req) => {\n"
                         << "        let ctx = Context.create(req);\n"
                         << "        return ctx.json(\"{\\\"status\\\":\\\"ok\\\",\\\"service\\\":\\\"" << projName << "\\\"}\");\n"
                         << "    });\n\n"
                         << "    app.get(\"/api/v1/info\", (ptr req) => {\n"
                         << "        let ctx = Context.create(req);\n"
                         << "        return ctx.json(\"{\\\"version\\\":\\\"1.0.0\\\",\\\"framework\\\":\\\"sp-web\\\",\\\"edition\\\":\\\"2028\\\"}\");\n"
                         << "    });\n\n"
                         << "    let port = 8080;\n"
                         << "    println(\"[+] " << projName << " listening at http://127.0.0.1:\" + port);\n"
                         << "    println(\"    * Swagger UI: http://127.0.0.1:\" + port + \"/docs\");\n"
                         << "    println(\"    * Redoc:      http://127.0.0.1:\" + port + \"/redoc\");\n"
                         << "    app.listen(port);\n"
                         << "}\n";
            } else {
                mainFile << "import std.io;\n\n"
                         << "fn main() {\n"
                         << "    println(\"Hello, SP World from " << projName << "!\");\n"
                         << "}\n";
            }
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

    if (templateType == "api") {
        // Create tests directory with in-memory HTTP integration test
        fs::path testsDir = pDir / "tests";
        fs::create_directories(testsDir, ec);
        fs::path testFilePath = testsDir / "api_test.sp";
        if (!fs::exists(testFilePath)) {
            std::ofstream testFile(testFilePath);
            testFile << "module tests.api_test;\n\n"
                     << "import std.io;\n"
                     << "import std.web;\n"
                     << "import std.test.http;\n\n"
                     << "test \"Health Endpoint Returns 200 OK\" {\n"
                     << "    let app = new App();\n"
                     << "    app.get(\"/api/health\", (ptr req) => {\n"
                     << "        let ctx = Context.create(req);\n"
                     << "        return ctx.json(\"{\\\"status\\\":\\\"ok\\\"}\");\n"
                     << "    });\n\n"
                     << "    let client = TestClient.fromApp(app);\n"
                     << "    let res = client.get(\"/api/health\");\n"
                     << "    res.assertOk();\n"
                     << "    res.assertBodyContains(\"status\");\n"
                     << "    res.destroy();\n"
                     << "    client.destroy();\n"
                     << "}\n";
        }

        // Create Dockerfile
        fs::path dockerfilePath = pDir / "Dockerfile";
        if (!fs::exists(dockerfilePath)) {
            std::ofstream dockerfile(dockerfilePath);
            dockerfile << "# Multi-stage minimal static container build for SP Language\n"
                       << "FROM debian:bookworm-slim AS builder\n"
                       << "WORKDIR /app\n"
                       << "COPY . .\n"
                       << "RUN spc build src/main.sp --static --release -o server\n\n"
                       << "FROM scratch\n"
                       << "WORKDIR /app\n"
                       << "COPY --from=builder /app/server /app/server\n"
                       << "EXPOSE 8080\n"
                       << "ENTRYPOINT [\"/app/server\"]\n";
        }

        // Create README.md
        fs::path readmePath = pDir / "README.md";
        if (!fs::exists(readmePath)) {
            std::ofstream readme(readmePath);
            readme << "# " << projName << "\n\n"
                   << "High-performance enterprise REST API service built with SP Language.\n\n"
                   << "## Development\n\n"
                   << "Start the auto-reloading development server:\n"
                   << "```bash\n"
                   << "spm dev\n"
                   << "```\n\n"
                   << "## Running Tests\n\n"
                   << "Execute the in-memory integration test suite without socket binding:\n"
                   << "```bash\n"
                   << "spm test\n"
                   << "```\n\n"
                   << "## OpenAPI 3.0 Documentation\n\n"
                   << "Generate OpenAPI 3.0.3 specification and Swagger UI documentation:\n"
                   << "```bash\n"
                   << "spc doc src/main.sp --swagger -o docs\n"
                   << "```\n\n"
                   << "## Production Container Build\n\n"
                   << "```bash\n"
                   << "docker build -t " << projName << " .\n"
                   << "docker run -p 8080:8080 " << projName << "\n"
                   << "```\n";
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
