#include "BuildOrchestrator.h"
#include "Publish.h"
#include "DependencyTree.h"
#include "Cache.h"
#include "PackageArchive.h"
#include "RegistryClient.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/runtime/Crypto.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/sema/ModuleLoader.h"
#include "sp/ast/Edition.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"
#include "sp/util/PathUtils.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace fs = std::filesystem;

namespace sp::spm {

std::string BuildOrchestrator::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string BuildOrchestrator::defaultRegistryUrl() {
    return HttpRegistryClient::resolveBaseUrl("");
}

namespace {

// Re-lock legacy registry checksums (pre-Phase-4 metadata hashes) with
// real tarball SHA-256 values, WITHOUT changing versions. The hash comes
// from the registry metadata when reachable, else from the cached archive
// bytes. Returns false when neither source can supply a hash — the
// lockfile is then left untouched and the caller fails loudly.
bool refreshLegacyChecksums(Lockfile& lock, RegistryProvider& provider, std::string& err) {
    Lockfile refreshed;
    for (const auto& pkg : lock.getPackages()) {
        LockedPackage entry = pkg;
        if (entry.source == "registry") {
            auto ver = Version::parse(entry.version);
            if (ver && entry.checksum ==
                          DependencyResolver::computeChecksum(entry.name, *ver, entry.source)) {
                std::string real;
                if (auto sum = provider.getPackageChecksum(entry.name, *ver)) {
                    real = *sum;
                } else {
                    std::string archiveBytes;
                    if (PackageCache::readCachedArchive(entry.name, entry.version, archiveBytes)) {
                        real = sp::rt::crypto::sha256(archiveBytes);
                    }
                }
                if (real.empty()) {
                    err = "cannot re-lock '" + entry.name + "@" + entry.version +
                          "' with a content hash: registry unreachable and no cached archive; "
                          "run 'spm install' with a reachable registry or restore the package cache";
                    return false;
                }
                entry.checksum = real;
            }
        }
        refreshed.addPackage(entry);
    }
    lock = refreshed;
    return true;
}

} // namespace

bool BuildOrchestrator::ensureVendored(const std::string& projectDir, const Lockfile& lock,
                                       const std::string& registryUrl, std::string& errorMessage) {
    auto client = std::make_shared<HttpRegistryClient>(registryUrl);
    return ensureVendored(projectDir, lock, client, errorMessage);
}

bool BuildOrchestrator::ensureVendored(const std::string& projectDir, const Lockfile& lock,
                                       std::shared_ptr<IRegistryClient> client,
                                       std::string& errorMessage) {
    fs::path vendorRoot = fs::path(projectDir) / ".spm" / "packages";
    for (const auto& pkg : lock.getPackages()) {
        if (pkg.source != "registry") continue;
        fs::path destDir = vendorRoot / pkg.name;

        // Fast path: already vendored with a matching checksum marker.
        if (fs::exists(destDir / "mod.sp")) {
            std::ifstream marker(destDir / ".sp-checksum");
            std::string recorded;
            if (marker.is_open()) std::getline(marker, recorded);
            if (recorded == pkg.checksum) continue;
        }

        // 1. Archive bytes: cache first (hash-verified), else the registry.
        std::string archiveBytes;
        bool haveBytes = false;
        if (PackageCache::readCachedArchive(pkg.name, pkg.version, archiveBytes)) {
            if (sp::rt::crypto::sha256(archiveBytes) == pkg.checksum) {
                haveBytes = true;
            } else {
                // Cached archive disagrees with sp.lock: do not trust it;
                // try the registry instead.
                archiveBytes.clear();
            }
        }
        if (!haveBytes) {
            std::string downloaded, downloadedChecksum, err;
            if (!client->download(pkg.name, pkg.version, downloaded, downloadedChecksum, err)) {
                errorMessage = "cannot vendor '" + pkg.name + "@" + pkg.version + "': " + err +
                               " (run 'spm install' with a reachable registry or restore the package cache)";
                return false;
            }
            if (downloadedChecksum != pkg.checksum) {
                errorMessage = "refusing to vendor '" + pkg.name + "@" + pkg.version +
                               "': registry tarball SHA-256 (" + downloadedChecksum +
                               ") does not match sp.lock (" + pkg.checksum + "). "
                               "If sp.lock predates Phase 4, run 'spm install' to re-lock with content hashes.";
                return false;
            }
            archiveBytes = std::move(downloaded);
            std::string storeErr;
            if (!PackageCache::storeArchive(pkg.name, pkg.version, archiveBytes, downloadedChecksum,
                                            storeErr)) {
                errorMessage = "cannot cache downloaded '" + pkg.name + "@" + pkg.version +
                               "': " + storeErr;
                return false;
            }
        }

        // 2. Parse + extract (wipe any stale tree first).
        PackageArchiveData data;
        std::string parseErr;
        if (!PackageArchive::parse(archiveBytes, data, parseErr)) {
            errorMessage = "cannot vendor '" + pkg.name + "@" + pkg.version +
                           "': unparsable archive: " + parseErr;
            return false;
        }
        std::error_code ec;
        fs::remove_all(destDir, ec);
        std::string extractErr;
        if (!PackageArchive::extract(data, destDir, pkg.checksum, extractErr)) {
            errorMessage = "cannot vendor '" + pkg.name + "@" + pkg.version + "': " + extractErr;
            return false;
        }
    }
    return true;
}

InstallResult BuildOrchestrator::install(const std::string& projectDir,
                                         const std::string& registryUrl,
                                         bool forceResolve,
                                         bool frozen) {
    auto client = std::make_shared<HttpRegistryClient>(registryUrl);
    return install(projectDir, client, forceResolve, frozen);
}

InstallResult BuildOrchestrator::install(const std::string& projectDir,
                                         std::shared_ptr<IRegistryClient> client,
                                         bool forceResolve,
                                         bool frozen) {
    InstallResult result;
    std::string url = client ? client->baseUrl() : HttpRegistryClient::resolveBaseUrl("");

    fs::path tomlPath = fs::path(projectDir) / "sp.toml";
    if (!fs::exists(tomlPath)) {
        result.errorMessage = "could not find 'sp.toml' in '" + projectDir + "'";
        return result;
    }
    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        result.errorMessage = "failed to load sp.toml: " + config.getLastError();
        return result;
    }

    auto provider = std::make_shared<RegistryProvider>(client);

    fs::path lockPath = fs::path(projectDir) / "sp.lock";
    Lockfile lock;
    bool haveLock = fs::exists(lockPath) && lock.loadFromFile(lockPath.string()) &&
                    lock.isUpToDate(config);

    if (haveLock && !forceResolve) {
        // Up-to-date lock: still vendor (a fresh checkout has sp.lock
        // but no .spm/packages), and upgrade any legacy metadata
        // checksums to content hashes without changing versions.
        bool legacyFound = false;
        for (const auto& pkg : lock.getPackages()) {
            if (pkg.source != "registry") continue;
            auto ver = Version::parse(pkg.version);
            if (ver && pkg.checksum ==
                          DependencyResolver::computeChecksum(pkg.name, *ver, pkg.source)) {
                legacyFound = true;
                break;
            }
        }
        if (legacyFound) {
            if (frozen) {
                result.errorMessage = "sp.lock uses legacy metadata checksums and --frozen was "
                                      "specified; run 'spm install' without --frozen to re-lock "
                                      "with content hashes";
                return result;
            }
            std::string refreshErr;
            if (!refreshLegacyChecksums(lock, *provider, refreshErr)) {
                result.errorMessage = refreshErr;
                return result;
            }
            if (!lock.saveToFile(lockPath.string())) {
                result.errorMessage = "failed to save sp.lock: " + lock.getLastError();
                return result;
            }
            std::cout << "sp.lock re-locked with content hashes (versions unchanged).\n";
        } else {
            std::cout << "All dependencies up-to-date (" << lock.getPackages().size()
                      << " packages locked).\n";
        }
    } else {
        if (frozen) {
            result.errorMessage = "sp.lock is out-of-sync with sp.toml and --frozen was specified";
            return result;
        }
        std::cout << "Resolving dependencies for '" << config.getPackage().name << "'...\n";
        DependencyResolver resolver(provider);
        auto res = resolver.resolve(config, projectDir, true);
        if (!res.success) {
            result.errorMessage = "dependency resolution failed: " + res.errorMessage;
            return result;
        }
        lock = Lockfile::fromResolution(res);
        if (!lock.saveToFile(lockPath.string())) {
            result.errorMessage = "failed to save sp.lock: " + lock.getLastError();
            return result;
        }
        std::cout << "Successfully resolved and locked " << res.resolvedPackages.size()
                  << " package(s):\n";
        for (const auto& pkg : res.resolvedPackages) {
            std::cout << "  - " << pkg.name << " v" << pkg.version.toString() << " ("
                      << sourceKindToString(pkg.sourceKind) << ")\n";
        }
    }

    if (provider->registryUnavailable()) {
        result.registryUnavailable = true;
        std::cout << "[spm] note: registry at " << url << " unreachable ("
                  << provider->lastRegistryError()
                  << "); resolved from the local package cache only.\n";
    }

    std::string vendorErr;
    if (!ensureVendored(projectDir, lock, client, vendorErr)) {
        result.errorMessage = vendorErr;
        return result;
    }
    size_t registryCount = 0;
    for (const auto& pkg : lock.getPackages()) {
        if (pkg.source == "registry") registryCount++;
    }
    result.packagesInstalled = registryCount;
    result.success = true;
    return result;
}

BuildResult BuildOrchestrator::buildWorkspace(const std::string& workspaceDir, const BuildOptions& options) {
    BuildResult result;
    fs::path tomlPath = fs::path(workspaceDir) / "sp.toml";
    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        result.errorMessage = "Failed to load workspace sp.toml: " + config.getLastError();
        return result;
    }

    if (!config.isWorkspace()) {
        result.errorMessage = "Project at '" + workspaceDir + "' is not a workspace";
        return result;
    }

    for (const auto& member : config.getWorkspaceMembers()) {
        fs::path memberDir = fs::path(workspaceDir) / member;
        BuildOptions memberOpts = options;
        memberOpts.targetPackage.clear();
        auto memberRes = build(memberDir.string(), memberOpts);
        if (!memberRes.success) {
            result.errorMessage = "Failed to build workspace member '" + member + "': " + memberRes.errorMessage;
            return result;
        }
        result.memberBinaries.push_back(memberRes.targetBinary);
    }

    result.success = true;
    if (!result.memberBinaries.empty()) {
        result.targetBinary = result.memberBinaries.back();
    }
    return result;
}

BuildResult BuildOrchestrator::build(const std::string& projectDir, const BuildOptions& options) {
    BuildResult result;
    fs::path tomlPath = fs::path(projectDir) / "sp.toml";

    if (!fs::exists(tomlPath)) {
        result.errorMessage = "Could not find 'sp.toml' in '" + projectDir + "'";
        return result;
    }

    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        result.errorMessage = "Failed to load 'sp.toml': " + config.getLastError();
        return result;
    }

    // Check if building workspace monorepo
    if (config.isWorkspace()) {
        if (!options.targetPackage.empty()) {
            for (const auto& member : config.getWorkspaceMembers()) {
                fs::path mDir = fs::path(projectDir) / member;
                ProjectConfig mCfg;
                if (mCfg.loadFromFile((mDir / "sp.toml").string()) && mCfg.getPackage().name == options.targetPackage) {
                    return build(mDir.string(), options);
                }
            }
            result.errorMessage = "Workspace member '" + options.targetPackage + "' not found in workspace";
            return result;
        }
        return buildWorkspace(projectDir, options);
    }

    // Lockfile and dependency resolution
    fs::path lockPath = fs::path(projectDir) / "sp.lock";
    Lockfile lock;
    bool needsResolution = false;

    if (!fs::exists(lockPath)) {
        needsResolution = true;
    } else {
        if (!lock.loadFromFile(lockPath.string()) || !lock.isUpToDate(config)) {
            needsResolution = true;
        }
    }

    if (needsResolution) {
        // Phase 4 slice A: resolve through the registry-first provider so
        // the lockfile pins real tarball SHA-256 checksums, not metadata
        // hashes — install()/ensureVendored() below verify against them.
        auto registryClient = std::make_shared<HttpRegistryClient>(defaultRegistryUrl());
        DependencyResolver resolver(std::make_shared<RegistryProvider>(registryClient));
        auto res = resolver.resolve(config, projectDir, false);
        if (!res.success) {
            result.errorMessage = "Dependency resolution failed: " + res.errorMessage;
            return result;
        }
        lock = Lockfile::fromResolution(res);
        if (!lock.saveToFile(lockPath.string())) {
            result.errorMessage = "Failed to save 'sp.lock': " + lock.getLastError();
            return result;
        }
    }

    // Phase 4 slice A: vendor registry dependencies before compiling —
    // a fresh checkout has sp.lock but no .spm/packages yet.
    if (!lock.getPackages().empty()) {
        std::string vendorErr;
        if (!ensureVendored(projectDir, lock, defaultRegistryUrl(), vendorErr)) {
            result.errorMessage = vendorErr;
            return result;
        }
    }

    // Determine entry file
    std::string entryRel = config.getPackage().entry;
    if (entryRel.empty()) {
        entryRel = (config.getPackage().packageType == "lib") ? "src/lib.sp" : "src/main.sp";
    }
    fs::path entryPath = fs::path(projectDir) / entryRel;
    if (!fs::exists(entryPath)) {
        result.errorMessage = "Entry point not found: " + entryPath.string();
        return result;
    }

    std::string sourceCode = readFile(entryPath.string());
    if (sourceCode.empty() && fs::file_size(entryPath) > 0) {
        result.errorMessage = "Failed to read entry file: " + entryPath.string();
        return result;
    }

    std::ostringstream diagStream;
    sp::diagnostics::DiagnosticEngine diagEngine(diagStream);

    sp::lexer::Lexer lexer(sourceCode, entryPath.string(), &diagEngine);
    auto tokens = lexer.tokenizeAll();
    if (diagEngine.hasErrors()) {
        result.errorMessage = "Lexical error during build:\n" + diagStream.str();
        return result;
    }

    sp::ast::Edition edition = sp::ast::parseEdition(config.getPackage().edition);
    sp::parser::Parser parser(tokens, entryPath.string(), &diagEngine);
    parser.setEdition(edition);
    auto program = parser.parseProgram();
    if (diagEngine.hasErrors() || !program) {
        result.errorMessage = "Syntax error during build:\n" + diagStream.str();
        return result;
    }

    sp::sema::ModuleLoader moduleLoader(diagEngine);
    moduleLoader.setEdition(edition);
    moduleLoader.addSearchPath(fs::path(projectDir) / "src");
    moduleLoader.addSearchPath(fs::path(projectDir));
    if (fs::exists(fs::path(projectDir) / ".spm" / "packages")) {
        moduleLoader.addSearchPath(fs::path(projectDir) / ".spm" / "packages");
    }
    if (!moduleLoader.loadAndMergeAll(*program, entryPath)) {
        result.errorMessage = "Module resolution error during build:\n" + diagStream.str();
        return result;
    }

    sp::sema::SemanticAnalyzer sema(diagEngine);
    sema.setEdition(edition);
    if (!sema.analyze(*program)) {
        result.errorMessage = "Semantic error during build:\n" + diagStream.str();
        return result;
    }

    std::string modName = config.getPackage().name;
    sp::ir::IRBuilder irBuilder(sema);
    auto irModule = irBuilder.build(*program, modName);
    if (!irModule) {
        result.errorMessage = "IR generation failed for module '" + modName + "'";
        return result;
    }

    sp::codegen::CodegenOptions codegenOpts;
    codegenOpts.releaseMode = options.releaseMode;
    codegenOpts.debugSymbols = options.debugMode;
    sp::codegen::LLVMGenerator llvmGen(codegenOpts);

    fs::path targetDir = fs::path(projectDir) / "target" / (options.releaseMode ? "release" : "debug");
    std::error_code ec;
    fs::create_directories(targetDir, ec);

    bool isLib = (config.getPackage().packageType == "lib");
    std::string binName = config.getPackage().name;
    if (isLib) {
#if defined(_WIN32)
        binName += ".obj";
#else
        binName += ".o";
#endif
    } else {
#if defined(_WIN32)
        binName += ".exe";
#endif
    }

    fs::path outBin = options.customOutput.empty() ? (targetDir / binName) : fs::path(options.customOutput);

    bool buildSuccess = isLib ? llvmGen.emitObjectFile(*irModule, outBin.string())
                              : llvmGen.compileToExecutable(*irModule, outBin.string());

    if (!buildSuccess) {
        result.errorMessage = "Compilation failed: " + llvmGen.getLastError();
        if (!sp::util::hasBackendToolchain()) {
            result.errorMessage += "\n[HINT] No backend C/LLVM compiler or linker was detected on this computer.\n"
                                   "       Run 'spc setup' to auto-provision a portable toolchain.";
        }
        return result;
    }

    result.success = true;
    result.targetBinary = outBin.string();
    return result;
}

int BuildOrchestrator::run(const std::string& projectDir, const BuildOptions& options, const std::vector<std::string>& runArgs) {
    auto res = build(projectDir, options);
    if (!res.success) {
        std::cerr << "error: build failed before run:\n" << res.errorMessage << "\n";
        return 1;
    }

    fs::path absBinary = fs::absolute(res.targetBinary);
    std::string runCmd = "\"" + absBinary.string() + "\"";
    for (const auto& arg : runArgs) {
        runCmd += " \"" + arg + "\"";
    }

    int exitCode = std::system(runCmd.c_str());
    return exitCode;
}

int BuildOrchestrator::dev(const std::string& projectDir, const BuildOptions& options, const std::vector<std::string>& runArgs) {
    fs::path pDir = fs::absolute(projectDir);
    std::cout << "[sp dev] Starting development server with hot-reload for: " << pDir.string() << "\n";

    auto getLatestModTime = [&](const fs::path& dir) -> fs::file_time_type {
        fs::file_time_type latestTime = fs::file_time_type::min();
        std::error_code ec;
        if (!fs::exists(dir, ec)) return latestTime;
        for (const auto& entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) continue;
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::string fname = entry.path().filename().string();
                if (ext == ".sp" || fname == "sp.toml") {
                    auto t = entry.last_write_time(ec);
                    if (!ec && t > latestTime) {
                        latestTime = t;
                    }
                }
            }
        }
        return latestTime;
    };

    auto lastCheckTime = getLatestModTime(pDir);

    auto res = build(projectDir, options);
    if (!res.success) {
        std::cerr << "error: initial build failed:\n" << res.errorMessage << "\n";
        std::cout << "[sp dev] Waiting for file changes to retry...\n";
    }

#if defined(_WIN32)
    PROCESS_INFORMATION procInfo{};
    bool hasChild = false;

    auto spawnProcess = [&](const std::string& binaryPath) -> bool {
        STARTUPINFOA si{};
        si.cb = sizeof(si);
        std::string cmd = "\"" + fs::absolute(binaryPath).string() + "\"";
        for (const auto& a : runArgs) {
            cmd += " \"" + a + "\"";
        }
        std::vector<char> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back('\0');

        BOOL success = CreateProcessA(
            NULL, cmdBuf.data(), NULL, NULL, FALSE,
            0, NULL, pDir.string().c_str(), &si, &procInfo
        );
        if (success) {
            hasChild = true;
            return true;
        }
        return false;
    };

    auto killChild = [&]() {
        if (hasChild && procInfo.hProcess) {
            TerminateProcess(procInfo.hProcess, 0);
            WaitForSingleObject(procInfo.hProcess, 100);
            CloseHandle(procInfo.hProcess);
            CloseHandle(procInfo.hThread);
            procInfo = {};
            hasChild = false;
        }
    };
#else
    pid_t childPid = -1;
    bool hasChild = false;

    auto spawnProcess = [&](const std::string& binaryPath) -> bool {
        pid_t pid = fork();
        if (pid == 0) {
            std::vector<char*> argvList;
            std::string absBin = fs::absolute(binaryPath).string();
            argvList.push_back(const_cast<char*>(absBin.c_str()));
            for (const auto& a : runArgs) {
                argvList.push_back(const_cast<char*>(a.c_str()));
            }
            argvList.push_back(nullptr);
            execvp(absBin.c_str(), argvList.data());
            _exit(1);
        } else if (pid > 0) {
            childPid = pid;
            hasChild = true;
            return true;
        }
        return false;
    };

    auto killChild = [&]() {
        if (hasChild && childPid > 0) {
            kill(childPid, SIGTERM);
            int status = 0;
            waitpid(childPid, &status, WNOHANG);
            childPid = -1;
            hasChild = false;
        }
    };
#endif

    if (res.success) {
        if (spawnProcess(res.targetBinary)) {
            std::cout << "[sp dev] Application started: " << res.targetBinary << "\n";
            std::cout << "[sp dev] Watching for changes in " << pDir.string() << " (Ctrl+C to quit)...\n";
        }
    }

    // Watch loop (runs until process interrupted)
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        auto curTime = getLatestModTime(pDir);
        if (curTime > lastCheckTime) {
            lastCheckTime = curTime;
            auto startReboot = std::chrono::high_resolution_clock::now();
            std::cout << "\n[sp dev] File change detected, recompiling...\n";

            killChild();

            auto rebuildRes = build(projectDir, options);
            if (rebuildRes.success) {
                if (spawnProcess(rebuildRes.targetBinary)) {
                    auto endReboot = std::chrono::high_resolution_clock::now();
                    double rebootMs = std::chrono::duration<double, std::milli>(endReboot - startReboot).count();
                    std::cout << "[sp dev] Server recompiled and restarted in " << std::fixed << std::setprecision(1) << rebootMs << " ms.\n";
                }
            } else {
                std::cerr << "error: build failed during hot-reload:\n" << rebuildRes.errorMessage << "\n";
                std::cout << "[sp dev] Waiting for fixes...\n";
            }
        }
    }

    killChild();
    return 0;
}

bool BuildOrchestrator::clean(const std::string& projectDir, std::string& errorMsg) {
    fs::path targetDir = fs::path(projectDir) / "target";
    if (fs::exists(targetDir)) {
        std::error_code ec;
        fs::remove_all(targetDir, ec);
        if (ec) {
            errorMsg = "Failed to remove target directory: " + ec.message();
            return false;
        }
    }
    return true;
}

PublishResult BuildOrchestrator::publish(const std::string& projectDir, const std::string& registryUrl) {
    PublishOptions opts;
    opts.registryUrl = registryUrl;
    std::string token = PackagePublisher::resolveApiToken(opts);
    opts.dryRun = token.empty();
    return PackagePublisher::publish(projectDir, opts);
}

PublishResult BuildOrchestrator::publish(const std::string& projectDir, const PublishOptions& options) {
    return PackagePublisher::publish(projectDir, options);
}

DependencyTreeResult BuildOrchestrator::tree(const std::string& projectDir, int maxDepth) {
    fs::path tomlPath = fs::path(projectDir) / "sp.toml";
    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        DependencyTreeResult res;
        res.success = false;
        res.formattedTree = "error: could not load sp.toml in " + projectDir;
        return res;
    }
    return DependencyTree::buildTree(config, projectDir, nullptr, maxDepth);
}

std::vector<SearchResult> BuildOrchestrator::search(const std::string& query, const std::string& registryUrl) {
    // Phase 4 slice A: query the real registry first. A reachable
    // registry is the source of truth; the static index below is only a
    // fallback when the registry cannot be reached, and the fallback is
    // announced rather than silent.
    std::string url = HttpRegistryClient::resolveBaseUrl(registryUrl);
    auto client = std::make_shared<HttpRegistryClient>(url);
    std::vector<RegistryPackageInfo> pkgs;
    std::string err;
    if (client->listPackages(pkgs, err)) {
        std::string qLower = query;
        std::transform(qLower.begin(), qLower.end(), qLower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::vector<SearchResult> matches;
        for (const auto& item : pkgs) {
            std::string nLower = item.name;
            std::string dLower = item.description;
            std::transform(nLower.begin(), nLower.end(), nLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            std::transform(dLower.begin(), dLower.end(), dLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (nLower.find(qLower) != std::string::npos ||
                dLower.find(qLower) != std::string::npos) {
                matches.push_back(SearchResult{item.name, item.version, item.description});
            }
        }
        return matches;
    }
    std::cerr << "[spm] warning: registry at " << url << " unreachable (" << err
              << "); showing the built-in offline index instead.\n";

    // Standard package index
    static const std::vector<SearchResult> registryIndex = {
        {"sp_math", "1.0.0", "Advanced mathematics, linear algebra, and statistical functions for SP"},
        {"sp_json", "0.8.2", "High-performance JSON parser and serializer with zero-copy decoding"},
        {"sp_http", "0.4.0", "Fast HTTP client and server library with async IO support"},
        {"sp_logger", "1.2.0", "Structured terminal and file logging utility for SP applications"},
        {"sp_test", "1.0.1", "BDD-style unit and benchmark testing framework for SP"},
        {"sp_crypto", "0.3.0", "Cryptographic primitives, SHA-256, AES, and secure hashing"},
        {"sp_csv", "0.2.1", "Fast CSV reader, writer, and typed data record parser"}
    };

    std::string qLower = query;
    std::transform(qLower.begin(), qLower.end(), qLower.begin(), [](unsigned char c) { return std::tolower(c); });

    std::vector<SearchResult> matches;
    for (const auto& item : registryIndex) {
        std::string nLower = item.name;
        std::string dLower = item.description;
        std::transform(nLower.begin(), nLower.end(), nLower.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(dLower.begin(), dLower.end(), dLower.begin(), [](unsigned char c) { return std::tolower(c); });

        if (nLower.find(qLower) != std::string::npos || dLower.find(qLower) != std::string::npos) {
            matches.push_back(item);
        }
    }
    return matches;
}

VerifyResult BuildOrchestrator::verify(const std::string& projectDir) {
    VerifyResult result;

    fs::path manifestPath = fs::path(projectDir) / "sp.toml";
    if (!fs::exists(manifestPath)) {
        result.errorMessage = "Missing manifest 'sp.toml' in directory: " + projectDir;
        return result;
    }

    ProjectConfig config;
    if (!config.loadFromFile(manifestPath.string())) {
        result.errorMessage = "Failed to parse 'sp.toml': " + config.getLastError();
        return result;
    }

    // Verify root package metadata
    if (config.getPackage().name.empty()) {
        result.errorMessage = "Package name cannot be empty in sp.toml";
        return result;
    }

    auto rootVersion = Version::parse(config.getPackage().version);
    if (!rootVersion) {
        result.errorMessage = "Invalid package version '" + config.getPackage().version + "' in sp.toml";
        return result;
    }

    fs::path lockPath = fs::path(projectDir) / "sp.lock";
    if (!fs::exists(lockPath)) {
        // Self-healing: auto-resolve and generate lockfile
        // Phase 4 slice A: resolve through the registry-first provider so
        // the generated lock pins real tarball SHA-256 checksums.
        auto registryClient = std::make_shared<HttpRegistryClient>(defaultRegistryUrl());
        DependencyResolver resolver(std::make_shared<RegistryProvider>(registryClient));
        auto res = resolver.resolve(config, projectDir, false);
        if (!res.success) {
            result.errorMessage = "Auto-generating 'sp.lock' failed: " + res.errorMessage;
            return result;
        }
        Lockfile autoLock = Lockfile::fromResolution(res);
        if (!autoLock.saveToFile(lockPath.string())) {
            result.errorMessage = "Failed to write 'sp.lock': " + autoLock.getLastError();
            return result;
        }
    }

    Lockfile lock;
    if (!lock.loadFromFile(lockPath.string())) {
        result.errorMessage = "Failed to load 'sp.lock': " + lock.getLastError();
        return result;
    }

    // Verify lockfile satisfies config
    if (!lock.isUpToDate(config)) {
        result.errorMessage = "sp.lock is out of sync with sp.toml. Run 'spm install' or 'spm update'.";
        return result;
    }

    // Verify each locked package's integrity & cryptographic checksum
    std::vector<std::string> seen;
    for (const auto& pkg : lock.getPackages()) {
        if (pkg.name.empty()) {
            result.errorMessage = "Found locked package with empty name";
            return result;
        }

        for (const auto& s : seen) {
            if (s == pkg.name) {
                result.errorMessage = "Duplicate package '" + pkg.name + "' detected in sp.lock";
                return result;
            }
        }
        seen.push_back(pkg.name);

        auto ver = Version::parse(pkg.version);
        if (!ver) {
            result.errorMessage = "Invalid version '" + pkg.version + "' for package '" + pkg.name + "' in sp.lock";
            return result;
        }

        // Validate cryptographic checksum
        if (pkg.checksum.empty()) {
            result.errorMessage = "Security audit failed: Missing checksum for package '" + pkg.name + "' in sp.lock";
            return result;
        }

        // Phase 4 slice A: registry entries pin the tarball CONTENT hash.
        // A legacy metadata hash (pre-Phase-4 lockfile) is not a content
        // check and fails loudly with a re-lock instruction; a content
        // hash is verified against the cached archive bytes, so a
        // tampered cache is detected instead of silently trusted.
        if (pkg.source == "registry") {
            if (pkg.checksum == DependencyResolver::computeChecksum(pkg.name, *ver, pkg.source)) {
                result.errorMessage = "Security audit failed: sp.lock for package '" + pkg.name +
                    "' uses a legacy metadata checksum, not a content hash. "
                    "Run 'spm install' to re-lock with content hashes.";
                return result;
            }
            std::string archiveBytes;
            if (!PackageCache::readCachedArchive(pkg.name, pkg.version, archiveBytes)) {
                result.errorMessage = "Security audit failed: package '" + pkg.name + "@" +
                    pkg.version + "' is not in the local cache; run 'spm install' to fetch and verify it";
                return result;
            }
            if (sp::rt::crypto::sha256(archiveBytes) != pkg.checksum) {
                result.errorMessage = "Security audit failed: Checksum mismatch / tampering detected for package '" +
                    pkg.name + "' (cached archive SHA-256 does not match sp.lock)";
                return result;
            }
        } else {
            // Verify checksum matches expected deterministic hash
            std::string expectedChecksum = DependencyResolver::computeChecksum(pkg.name, *ver, pkg.source);
            if (pkg.checksum != expectedChecksum) {
                result.errorMessage = "Security audit failed: Checksum mismatch / tampering detected for package '" +
                                      pkg.name + "' (expected: " + expectedChecksum + ", recorded: " + pkg.checksum + ")";
                return result;
            }
        }

        result.packagesVerified++;
    }

    result.success = true;
    return result;
}

} // namespace sp::spm

