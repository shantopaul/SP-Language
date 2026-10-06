#include "ProjectConfig.h"
#include "DependencyResolver.h"
#include "Lockfile.h"
#include "BuildOrchestrator.h"
#include <iostream>
#include <string_view>
#include <vector>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

namespace {

void printVersion() {
    std::cout << "spm (SP Package Manager) version 0.8.0-alpha\n"
              << "Copyright (c) 2026 Shanto Paul\n";
}

void printHelp() {
    std::cout << "Usage: spm [command] [options]\n\n"
              << "Commands:\n"
              << "  init [path]         Initialize a new SP project (default: binary app)\n"
              << "  add <package>       Add a dependency to sp.toml\n"
              << "  remove <package>    Remove a dependency from sp.toml\n"
              << "  install             Install and resolve locked dependencies\n"
              << "  update              Update project dependencies according to sp.toml\n"
              << "  build               Compile current package and its dependencies\n"
              << "  run [args...]       Compile and immediately run package binary\n"
              << "  clean               Remove build artifacts (target/ directory)\n"
              << "  publish             Publish current package to registry\n"
              << "  search <query>      Search for packages in registry\n\n"
              << "Options:\n"
              << "  --lib               Scaffold a library package instead of application binary\n"
              << "  --dev               Specify dependency as a development dependency\n"
              << "  --version <ver>     Specify version requirement when adding dependency\n"
              << "  --path <path>       Specify local path for dependency\n"
              << "  --git <url>         Specify git repository for dependency\n"
              << "  --release           Build/run with optimizations enabled\n"
              << "  --debug             Build/run with debug symbols\n"
              << "  --frozen            Require that sp.lock is up-to-date (used in CI)\n"
              << "  -o <output>         Specify custom output path\n"
              << "  -h, --help          Show help message\n"
              << "  -v, --version       Show version information\n";
}

int handleInit(int argc, char* argv[]) {
    std::string path = ".";
    std::string name;
    bool isLib = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--lib") {
            isLib = true;
        } else if (arg == "--name" && i + 1 < argc) {
            name = argv[++i];
        } else if (!arg.starts_with("-")) {
            path = arg;
        }
    }

    if (name.empty()) {
        fs::path p = fs::absolute(path);
        name = p.filename().string();
        if (name == "." || name.empty()) {
            name = "my_project";
        }
    }

    std::string errorMsg;
    if (!sp::spm::scaffoldProject(path, name, isLib, errorMsg)) {
        std::cerr << "error: " << errorMsg << "\n";
        return 1;
    }

    std::cout << "Created " << (isLib ? "library" : "binary (application)")
              << " package '" << name << "' at " << path << "\n";
    return 0;
}

int handleAdd(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: 'spm add' requires a package name\n"
                  << "Usage: spm add <package> [--version <req>] [--path <path>] [--git <url>] [--dev]\n";
        return 1;
    }

    std::string pkgName;
    std::string versionReq = "*";
    std::string path;
    std::string gitUrl;
    bool isDev = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--dev") {
            isDev = true;
        } else if ((arg == "--version" || arg == "-v") && i + 1 < argc) {
            versionReq = argv[++i];
        } else if (arg == "--path" && i + 1 < argc) {
            path = argv[++i];
        } else if (arg == "--git" && i + 1 < argc) {
            gitUrl = argv[++i];
        } else if (!arg.starts_with("-")) {
            if (pkgName.empty()) {
                pkgName = arg;
            }
        }
    }

    if (pkgName.empty()) {
        std::cerr << "error: missing package name to add\n";
        return 1;
    }

    fs::path tomlPath = "sp.toml";
    if (!fs::exists(tomlPath)) {
        std::cerr << "error: could not find 'sp.toml' in current directory\n"
                  << "Run 'spm init' to initialize an SP project first.\n";
        return 1;
    }

    sp::spm::ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        std::cerr << "error: failed to load sp.toml: " << config.getLastError() << "\n";
        return 1;
    }

    sp::spm::Dependency dep;
    dep.name = pkgName;
    dep.versionReq = versionReq;
    dep.path = path;
    dep.gitUrl = gitUrl;

    if (!config.addDependency(dep, isDev)) {
        std::cerr << "error: failed to add dependency '" << pkgName << "'\n";
        return 1;
    }

    if (!config.saveToFile(tomlPath.string())) {
        std::cerr << "error: failed to save sp.toml: " << config.getLastError() << "\n";
        return 1;
    }

    std::cout << "Added " << (isDev ? "dev-dependency" : "dependency")
              << " '" << pkgName << "'";
    if (!path.empty()) {
        std::cout << " (path: " << path << ")";
    } else if (!gitUrl.empty()) {
        std::cout << " (git: " << gitUrl << ")";
    } else {
        std::cout << " (version: " << versionReq << ")";
    }
    std::cout << " to sp.toml\n";

    return 0;
}

int handleRemove(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: 'spm remove' requires a package name\n"
                  << "Usage: spm remove <package> [--dev]\n";
        return 1;
    }

    std::string pkgName;
    bool isDev = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--dev") {
            isDev = true;
        } else if (!arg.starts_with("-")) {
            if (pkgName.empty()) {
                pkgName = arg;
            }
        }
    }

    if (pkgName.empty()) {
        std::cerr << "error: missing package name to remove\n";
        return 1;
    }

    fs::path tomlPath = "sp.toml";
    if (!fs::exists(tomlPath)) {
        std::cerr << "error: could not find 'sp.toml' in current directory\n";
        return 1;
    }

    sp::spm::ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        std::cerr << "error: failed to load sp.toml: " << config.getLastError() << "\n";
        return 1;
    }

    bool removed = config.removeDependency(pkgName, isDev);
    if (!removed && !isDev) {
        removed = config.removeDependency(pkgName, true);
    }

    if (!removed) {
        std::cerr << "error: dependency '" << pkgName << "' not found in sp.toml\n";
        return 1;
    }

    if (!config.saveToFile(tomlPath.string())) {
        std::cerr << "error: failed to save sp.toml: " << config.getLastError() << "\n";
        return 1;
    }

    std::cout << "Removed dependency '" << pkgName << "' from sp.toml\n";
    return 0;
}

int handleInstall(int argc, char* argv[]) {
    bool frozen = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--frozen") {
            frozen = true;
        }
    }

    fs::path tomlPath = "sp.toml";
    if (!fs::exists(tomlPath)) {
        std::cerr << "error: could not find 'sp.toml' in current directory\n";
        return 1;
    }

    sp::spm::ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        std::cerr << "error: failed to load sp.toml: " << config.getLastError() << "\n";
        return 1;
    }

    fs::path lockPath = "sp.lock";
    if (fs::exists(lockPath)) {
        sp::spm::Lockfile lock;
        if (lock.loadFromFile(lockPath.string()) && lock.isUpToDate(config)) {
            std::cout << "All dependencies up-to-date (" << lock.getPackages().size() << " packages locked).\n";
            return 0;
        }
        if (frozen) {
            std::cerr << "error: sp.lock is out-of-sync with sp.toml and --frozen was specified\n";
            return 1;
        }
    }

    std::cout << "Resolving dependencies for '" << config.getPackage().name << "'...\n";
    sp::spm::DependencyResolver resolver;
    auto res = resolver.resolve(config, ".", true);
    if (!res.success) {
        std::cerr << "error: dependency resolution failed: " << res.errorMessage << "\n";
        return 1;
    }

    auto lock = sp::spm::Lockfile::fromResolution(res);
    if (!lock.saveToFile(lockPath.string())) {
        std::cerr << "error: failed to save sp.lock: " << lock.getLastError() << "\n";
        return 1;
    }

    std::cout << "Successfully resolved and locked " << res.resolvedPackages.size() << " package(s):\n";
    for (const auto& pkg : res.resolvedPackages) {
        std::cout << "  - " << pkg.name << " v" << pkg.version.toString()
                  << " (" << sp::spm::sourceKindToString(pkg.sourceKind) << ")\n";
    }

    return 0;
}

int handleUpdate(int /*argc*/, char* /*argv*/[]) {
    fs::path tomlPath = "sp.toml";
    if (!fs::exists(tomlPath)) {
        std::cerr << "error: could not find 'sp.toml' in current directory\n";
        return 1;
    }

    sp::spm::ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        std::cerr << "error: failed to load sp.toml: " << config.getLastError() << "\n";
        return 1;
    }

    std::cout << "Updating dependencies for '" << config.getPackage().name << "'...\n";
    sp::spm::DependencyResolver resolver;
    auto res = resolver.resolve(config, ".", true);
    if (!res.success) {
        std::cerr << "error: dependency resolution failed: " << res.errorMessage << "\n";
        return 1;
    }

    auto lock = sp::spm::Lockfile::fromResolution(res);
    if (!lock.saveToFile("sp.lock")) {
        std::cerr << "error: failed to save sp.lock: " << lock.getLastError() << "\n";
        return 1;
    }

    std::cout << "Updated " << res.resolvedPackages.size() << " package(s) in sp.lock:\n";
    for (const auto& pkg : res.resolvedPackages) {
        std::cout << "  - " << pkg.name << " v" << pkg.version.toString()
                  << " (" << sp::spm::sourceKindToString(pkg.sourceKind) << ")\n";
    }

    return 0;
}

int handleBuild(int argc, char* argv[]) {
    sp::spm::BuildOptions opts;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--release") {
            opts.releaseMode = true;
        } else if (arg == "--debug") {
            opts.debugMode = true;
        } else if (arg == "-o" && i + 1 < argc) {
            opts.customOutput = argv[++i];
        }
    }

    std::cout << "Compiling project (" << (opts.releaseMode ? "release" : "debug") << " mode)...\n";
    auto res = sp::spm::BuildOrchestrator::build(".", opts);
    if (!res.success) {
        std::cerr << "error: " << res.errorMessage << "\n";
        return 1;
    }

    std::cout << "Finished " << (opts.releaseMode ? "release [optimized]" : "debug")
              << " target: " << res.targetBinary << "\n";
    return 0;
}

int handleRun(int argc, char* argv[]) {
    sp::spm::BuildOptions opts;
    std::vector<std::string> runArgs;
    bool collectingArgs = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (collectingArgs) {
            runArgs.push_back(std::string(arg));
        } else if (arg == "--") {
            collectingArgs = true;
        } else if (arg == "--release") {
            opts.releaseMode = true;
        } else if (arg == "--debug") {
            opts.debugMode = true;
        } else {
            runArgs.push_back(std::string(arg));
        }
    }

    return sp::spm::BuildOrchestrator::run(".", opts, runArgs);
}

int handleClean(int /*argc*/, char* /*argv*/[]) {
    std::string err;
    if (!sp::spm::BuildOrchestrator::clean(".", err)) {
        std::cerr << "error: " << err << "\n";
        return 1;
    }
    std::cout << "Removed target/ directory.\n";
    return 0;
}

int handlePublish(int argc, char* argv[]) {
    std::string registryUrl = "https://registry.sp-lang.org";
    for (int i = 2; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--registry" && i + 1 < argc) {
            registryUrl = argv[++i];
        }
    }

    std::cout << "Verifying package before publishing to " << registryUrl << "...\n";
    auto res = sp::spm::BuildOrchestrator::publish(".", registryUrl);
    if (!res.success) {
        std::cerr << "error: publication failed: " << res.errorMessage << "\n";
        return 1;
    }

    std::cout << "Package successfully verified and packaged: " << res.packageArtifact << "\n"
              << "Published to " << registryUrl << "\n";
    return 0;
}

int handleSearch(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: 'spm search' requires a query string\n"
                  << "Usage: spm search <query>\n";
        return 1;
    }

    std::string query = argv[2];
    auto results = sp::spm::BuildOrchestrator::search(query);

    std::cout << "Search results for '" << query << "' (" << results.size() << " match"
              << (results.size() == 1 ? "" : "es") << "):\n\n";

    for (const auto& item : results) {
        std::cout << "  " << std::left << std::setw(16) << item.name
                  << " v" << std::setw(8) << item.version
                  << " " << item.description << "\n";
    }

    if (results.empty()) {
        std::cout << "  No packages found matching '" << query << "'.\n";
    }

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::string_view cmd = argv[1];

    if (cmd == "-v" || cmd == "--version" || cmd == "version") {
        printVersion();
        return 0;
    }

    if (cmd == "-h" || cmd == "--help" || cmd == "help") {
        printHelp();
        return 0;
    }

    if (cmd == "init") {
        return handleInit(argc, argv);
    }

    if (cmd == "add") {
        return handleAdd(argc, argv);
    }

    if (cmd == "remove") {
        return handleRemove(argc, argv);
    }

    if (cmd == "install") {
        return handleInstall(argc, argv);
    }

    if (cmd == "update") {
        return handleUpdate(argc, argv);
    }

    if (cmd == "build") {
        return handleBuild(argc, argv);
    }

    if (cmd == "run") {
        return handleRun(argc, argv);
    }

    if (cmd == "clean") {
        return handleClean(argc, argv);
    }

    if (cmd == "publish") {
        return handlePublish(argc, argv);
    }

    if (cmd == "search") {
        return handleSearch(argc, argv);
    }

    std::cerr << "error: unknown or not-yet-implemented command: " << cmd << "\n"
              << "Run 'spm --help' for a list of available commands.\n";
    return 1;
}
