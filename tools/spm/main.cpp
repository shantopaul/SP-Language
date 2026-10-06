#include "ProjectConfig.h"
#include <iostream>
#include <string_view>
#include <vector>
#include <filesystem>

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
              << "  update              Update project dependencies according to sp.toml\n"
              << "  install             Install and resolve locked dependencies\n"
              << "  build               Compile current package and its dependencies\n"
              << "  run [args...]       Compile and immediately run package binary\n"
              << "  test                Run project unit and integration tests\n"
              << "  clean               Remove build artifacts (target/ directory)\n"
              << "  publish             Publish current package to registry\n"
              << "  search <query>      Search for packages in registry\n\n"
              << "Options:\n"
              << "  --lib               Scaffold a library package instead of application binary\n"
              << "  --dev               Specify dependency as a development dependency\n"
              << "  --version <ver>     Specify version requirement when adding dependency\n"
              << "  --path <path>       Specify local path for dependency\n"
              << "  --release           Build/run with optimizations enabled\n"
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
              << " '" << name << "' package in " << path << "\n";
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

    std::cerr << "error: unknown or not-yet-implemented command: " << cmd << "\n"
              << "Run 'spm --help' for a list of available commands.\n";
    return 1;
}
