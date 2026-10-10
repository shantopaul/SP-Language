#include "ProjectConfig.h"
#include "DependencyResolver.h"
#include "Lockfile.h"
#include "BuildOrchestrator.h"
#include "Cache.h"
#include "Publish.h"
#include "DependencyTree.h"
#include "sp/Version.h"
#include <iostream>
#include <string_view>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {

void printVersion() {
    std::cout << "spm (SP Package Manager) version " << sp::SP_VERSION_STRING << "\n"
              << "Copyright (c) 2026 Shanto Paul\n"
              << "Registry: " << sp::SP_REGISTRY_URL << "\n"
              << "Docs:     " << sp::SP_DOCS_URL << "\n";
}

void printHelp() {
    std::cout << "Usage: spm [command] [options]\n\n"
              << "Commands:\n"
              << "  new <name>          Create a new SP project in a new directory\n"
              << "  init [path]         Initialize a new SP project (default: binary app)\n"
              << "  dev [args...]       Run in development mode with automatic hot-reloading\n"
              << "  add <package>       Add a dependency to sp.toml\n"
              << "  remove <package>    Remove a dependency from sp.toml\n"
              << "  install             Install and resolve locked dependencies\n"
              << "  update              Update project dependencies according to sp.toml\n"
              << "  build               Compile current package and its dependencies\n"
              << "  run [args...]       Compile and immediately run package binary\n"
              << "  clean               Remove build artifacts (target/ directory)\n"
              << "  verify              Verify package lockfile integrity and cryptographic checksums\n"
              << "  tree                Display transitive dependency tree and version conflict diagnostics\n"
              << "  cache [list|clean]  Manage local offline package cache (~/.sp/cache/packages/)\n"
              << "  publish             Publish current package to registry\n"
              << "  search <query>      Search for packages in registry\n\n"
              << "Options:\n"
              << "  --template <type>   Project template: 'api' (REST backend) or 'default'\n"
              << "  --lib               Scaffold a library package instead of application binary\n"
              << "  --dev               Specify dependency as a development dependency\n"
              << "  --version <ver>     Specify version requirement when adding dependency\n"
              << "  --path <path>       Specify local path for dependency\n"
              << "  --git <url>         Specify git repository for dependency\n"
              << "  --release           Build/run with optimizations enabled\n"
              << "  --debug             Build/run with debug symbols\n"
              << "  --frozen            Require that sp.lock is up-to-date (used in CI)\n"
              << "  --workspace         Build all workspace members in a monorepo\n"
              << "  -p, --package <pkg> Target specific workspace member package\n"
              << "  --dry-run           Package and verify without uploading to registry\n"
              << "  --token <token>     API token for registry authentication\n"
              << "  -o <output>         Specify custom output path\n"
              << "  -h, --help          Show help message\n"
              << "  -v, --version       Show version information\n";
}

int handleNew(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: 'spm new' requires a project name\n"
                  << "Usage: spm new <name> [--template <api|default>] [--lib]\n";
        return 1;
    }
    std::string name;
    std::string templateType;
    bool isLib = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--lib") {
            isLib = true;
        } else if ((arg == "--template" || arg == "-t") && i + 1 < argc) {
            templateType = argv[++i];
        } else if (!arg.starts_with("-")) {
            if (name.empty()) {
                name = arg;
            }
        }
    }

    if (name.empty()) {
        std::cerr << "error: missing project name for 'spm new'\n";
        return 1;
    }

    std::string path = name;
    std::string errorMsg;
    if (!sp::spm::scaffoldProject(path, name, isLib, errorMsg, templateType)) {
        std::cerr << "error: " << errorMsg << "\n";
        return 1;
    }

    std::cout << "Created " << (isLib ? "library" : (templateType == "api" ? "REST API" : "binary (application)"))
              << " package '" << name << "' at ./" << path << "\n";
    return 0;
}

int handleInit(int argc, char* argv[]) {
    std::string path = ".";
    std::string name;
    std::string templateType;
    bool isLib = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--lib") {
            isLib = true;
        } else if ((arg == "--template" || arg == "-t") && i + 1 < argc) {
            templateType = argv[++i];
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
    if (!sp::spm::scaffoldProject(path, name, isLib, errorMsg, templateType)) {
        std::cerr << "error: " << errorMsg << "\n";
        return 1;
    }

    std::cout << "Created " << (isLib ? "library" : (templateType == "api" ? "REST API" : "binary (application)"))
              << " package '" << name << "' at " << path << "\n";
    return 0;
}

int handleDev(int argc, char* argv[]) {
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

    return sp::spm::BuildOrchestrator::dev(".", opts, runArgs);
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

    // Parse package name and version constraint if written as name@version
    auto atPos = pkgName.find('@');
    if (atPos != std::string::npos) {
        if (versionReq == "*") {
            versionReq = pkgName.substr(atPos + 1);
        }
        pkgName = pkgName.substr(0, atPos);
    }

    static const std::unordered_set<std::string> stdlibModules = {
        "json", "io", "fs", "net", "http", "db", "crypto", "time", "math",
        "collections", "string", "process", "sync", "web", "redis", "queue",
        "log", "metrics", "health", "security", "mem", "test", "ws", "sse",
        "std.json", "std.io", "std.fs", "std.net", "std.http", "std.db", "std.crypto", "std.time", "std.math",
        "std.collections", "std.string", "std.process", "std.sync", "std.web", "std.redis", "std.queue",
        "std.log", "std.metrics", "std.health", "std.security", "std.mem", "std.test", "std.ws", "std.sse"
    };

    if (stdlibModules.count(pkgName)) {
        std::string modName = pkgName.starts_with("std.") ? pkgName.substr(4) : pkgName;
        std::cout << "[NOTE] '" << pkgName << "' is a built-in SP Standard Library module ('import std." 
                  << modName << ";').\n"
                  << "       You can use it directly in your code without adding an external dependency!\n\n";
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

    // Auto-update lockfile
    fs::path lockPath = "sp.lock";
    sp::spm::DependencyResolver resolver;
    auto res = resolver.resolve(config, ".", true);
    if (res.success) {
        auto lock = sp::spm::Lockfile::fromResolution(res);
        lock.saveToFile(lockPath.string());
    }

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
        } else if ((arg == "-p" || arg == "--package") && i + 1 < argc) {
            opts.targetPackage = argv[++i];
        }
    }

    std::cout << "Compiling project (" << (opts.releaseMode ? "release" : "debug") << " mode)...\n";
    auto res = sp::spm::BuildOrchestrator::build(".", opts);
    if (!res.success) {
        std::cerr << "error: " << res.errorMessage << "\n";
        return 1;
    }

    if (!res.memberBinaries.empty()) {
        std::cout << "Finished workspace monorepo build (" << res.memberBinaries.size() << " packages compiled):\n";
        for (const auto& bin : res.memberBinaries) {
            std::cout << "  - " << bin << "\n";
        }
    } else {
        std::cout << "Finished " << (opts.releaseMode ? "release [optimized]" : "debug")
                  << " target: " << res.targetBinary << "\n";
    }
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

int handleVerify(int /*argc*/, char* /*argv*/[]) {
    auto res = sp::spm::BuildOrchestrator::verify(".");
    if (!res.success) {
        std::cerr << "error: verification failed: " << res.errorMessage << "\n";
        return 1;
    }
    std::cout << "Verified: package integrity and lockfile checksums valid ("
              << res.packagesVerified << " package" << (res.packagesVerified == 1 ? "" : "s")
              << " verified, 0 tampering detected).\n";
    return 0;
}

int handlePublish(int argc, char* argv[]) {
    sp::spm::PublishOptions opts;
    opts.registryUrl = "https://registry.splang.shantopaul.com";

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--registry" && i + 1 < argc) {
            opts.registryUrl = argv[++i];
        } else if (arg == "--token" && i + 1 < argc) {
            opts.token = argv[++i];
        } else if (arg == "--dry-run") {
            opts.dryRun = true;
        } else if (arg == "--skip-build") {
            opts.skipBuildValidation = true;
        }
    }

    std::cout << "Packaging and verifying release for " << opts.registryUrl
              << (opts.dryRun ? " (dry-run mode)" : "") << "...\n";
    auto res = sp::spm::BuildOrchestrator::publish(".", opts);
    if (!res.success) {
        std::cerr << "error: publication failed: " << res.errorMessage << "\n";
        return 1;
    }

    std::cout << "Package:    " << res.packageName << " v" << res.packageVersion << "\n"
              << "Artifact:   " << res.packageArtifact << " (" << res.archiveSizeBytes << " bytes)\n"
              << "Checksum:   " << res.sha256Checksum << " (SHA-256)\n"
              << "Status:     " << res.registryResponse << "\n";
    return 0;
}

int handleTree(int argc, char* argv[]) {
    int maxDepth = 100;
    bool asciiOnly = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--depth" && i + 1 < argc) {
            maxDepth = std::atoi(argv[++i]);
        } else if (arg == "--ascii") {
            asciiOnly = true;
        }
    }

    auto treeRes = sp::spm::BuildOrchestrator::tree(".", maxDepth);
    if (!treeRes.success) {
        std::cerr << "error: " << treeRes.formattedTree << "\n";
        for (const auto& w : treeRes.conflictWarnings) {
            std::cerr << "  " << w << "\n";
        }
        return 1;
    }

    std::cout << (asciiOnly ? sp::spm::DependencyTree::formatTree(treeRes.root, false) : treeRes.formattedTree);

    if (!treeRes.conflictWarnings.empty()) {
        std::cout << "\n[!] Version Conflicts & Warnings Detected (" << treeRes.conflictWarnings.size() << "):\n";
        for (const auto& w : treeRes.conflictWarnings) {
            std::cout << "  - " << w << "\n";
        }
    }
    return 0;
}

int handleCache(int argc, char* argv[]) {
    std::string sub = (argc >= 3) ? argv[2] : "list";

    if (sub == "clean" || sub == "clear") {
        std::string err;
        if (!sp::spm::PackageCache::clean(err)) {
            std::cerr << "error: failed to clean cache: " << err << "\n";
            return 1;
        }
        std::cout << "Package cache cleaned successfully.\n";
        return 0;
    }

    if (sub == "dir" || sub == "path") {
        std::cout << sp::spm::PackageCache::getCacheRootDir().string() << "\n";
        return 0;
    }

    // Default: list
    auto pkgs = sp::spm::PackageCache::listCachedPackages();
    size_t bytes = sp::spm::PackageCache::getCacheSizeBytes();
    double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);

    std::cout << "SP Offline Package Cache: " << sp::spm::PackageCache::getPackagesRootDir().string() << "\n"
              << "Total Cached: " << pkgs.size() << " packages (" << std::fixed << std::setprecision(2) << mb << " MB)\n\n";

    for (const auto& [name, ver] : pkgs) {
        std::string cks = sp::spm::PackageCache::getCachedChecksum(name, ver);
        std::cout << "  - " << std::left << std::setw(20) << (name + " v" + ver);
        if (!cks.empty()) {
            std::cout << " [SHA-256: " << cks.substr(0, 16) << "...]";
        }
        std::cout << "\n";
    }

    if (pkgs.empty()) {
        std::cout << "  (Cache is currently empty)\n";
    }

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

    if (cmd == "new") {
        return handleNew(argc, argv);
    }

    if (cmd == "init") {
        return handleInit(argc, argv);
    }

    if (cmd == "dev") {
        return handleDev(argc, argv);
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

    if (cmd == "verify") {
        return handleVerify(argc, argv);
    }

    if (cmd == "tree") {
        return handleTree(argc, argv);
    }

    if (cmd == "cache") {
        return handleCache(argc, argv);
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
