#include "Publish.h"
#include "RegistryClient.h"
#include "sp/runtime/Crypto.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/Version.h"
#include "Cache.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;

namespace sp::spm {

namespace {

bool isValidPackageName(const std::string& name) {
    if (name.size() < 2 || name.size() > 64) return false;
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

std::string readFileContents(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

std::string PackagePublisher::computeFileSha256(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    return sp::rt::crypto::sha256(content);
}

std::string PackagePublisher::resolveApiToken(const PublishOptions& options) {
    if (!options.token.empty()) {
        return options.token;
    }
    const char* envToken = std::getenv("SP_REGISTRY_TOKEN");
    if (envToken && *envToken) {
        return std::string(envToken);
    }

    // Check credentials file at ~/.sp/credentials or $SP_HOME/credentials
    fs::path credPath;
    const char* envHome = std::getenv("SP_HOME");
    if (envHome && *envHome) {
        credPath = fs::path(envHome) / "credentials";
    } else {
#ifdef _WIN32
        const char* userProfile = std::getenv("USERPROFILE");
        if (userProfile && *userProfile) {
            credPath = fs::path(userProfile) / ".sp" / "credentials";
        }
#else
        const char* home = std::getenv("HOME");
        if (home && *home) {
            credPath = fs::path(home) / ".sp" / "credentials";
        }
#endif
    }

    if (!credPath.empty() && fs::exists(credPath)) {
        std::ifstream credFile(credPath);
        std::string line;
        while (std::getline(credFile, line)) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq + 1);
                // trim
                while (!k.empty() && std::isspace(k.front())) k.erase(k.begin());
                while (!k.empty() && std::isspace(k.back())) k.pop_back();
                while (!v.empty() && std::isspace(v.front())) v.erase(v.begin());
                while (!v.empty() && std::isspace(v.back())) v.pop_back();
                if (k == "token" || k == "registry_token") {
                    return v;
                }
            }
        }
    }

    return "";
}

PublishResult PackagePublisher::publish(const std::string& projectDir, const PublishOptions& options) {
    PublishResult result;

    fs::path tomlPath = fs::path(projectDir) / "sp.toml";
    if (!fs::exists(tomlPath)) {
        result.errorMessage = "Missing package manifest 'sp.toml' in: " + projectDir;
        return result;
    }

    ProjectConfig config;
    if (!config.loadFromFile(tomlPath.string())) {
        result.errorMessage = "Failed to parse sp.toml: " + config.getLastError();
        return result;
    }

    const auto& pkg = config.getPackage();
    result.packageName = pkg.name;
    result.packageVersion = pkg.version;

    // 1. Validate manifest fields
    if (pkg.name.empty() || !isValidPackageName(pkg.name)) {
        result.errorMessage = "Invalid package name '" + pkg.name + "'. Names must be 2-64 chars [a-z0-9_-].";
        return result;
    }

    if (pkg.version.empty()) {
        result.errorMessage = "Missing package version in sp.toml";
        return result;
    }

    fs::path srcDir = fs::path(projectDir) / "src";
    if (!fs::exists(srcDir) || !fs::is_directory(srcDir)) {
        result.errorMessage = "Cannot publish package without 'src/' directory containing code.";
        return result;
    }

    // 2. Validate source compilation
    if (!options.skipBuildValidation) {
        std::string entryRel = pkg.entry.empty() ? (pkg.packageType == "lib" ? "src/lib.sp" : "src/main.sp") : pkg.entry;
        fs::path entryPath = fs::path(projectDir) / entryRel;
        if (fs::exists(entryPath)) {
            std::string code = readFileContents(entryPath);
            std::ostringstream diagStream;
            sp::diagnostics::DiagnosticEngine diagEngine(diagStream);
            sp::lexer::Lexer lexer(code, entryPath.string(), &diagEngine);
            auto tokens = lexer.tokenizeAll();
            if (diagEngine.hasErrors()) {
                result.errorMessage = "Compilation pre-check failed (lexical errors):\n" + diagStream.str();
                return result;
            }
            sp::parser::Parser parser(tokens, entryPath.string(), &diagEngine);
            auto ast = parser.parseProgram();
            if (diagEngine.hasErrors() || !ast) {
                result.errorMessage = "Compilation pre-check failed (syntax errors):\n" + diagStream.str();
                return result;
            }
        }
    }

    // 3. Automated package packaging & tarball generation
    fs::path outDir = fs::path(projectDir) / "target" / "packages";
    std::error_code ec;
    fs::create_directories(outDir, ec);

    std::string archiveName = pkg.name + "-" + pkg.version + ".tar.gz";
    fs::path archivePath = outDir / archiveName;

    // Assemble deterministic tar archive content
    std::ostringstream archiveStream;
    archiveStream << "SP_PACKAGE_ARCHIVE_V1\n";
    archiveStream << "name=" << pkg.name << "\n";
    archiveStream << "version=" << pkg.version << "\n";
    archiveStream << "edition=" << pkg.edition << "\n";
    archiveStream << "entry=" << pkg.entry << "\n";
    archiveStream << "type=" << pkg.packageType << "\n";
    archiveStream << "---MANIFEST---\n";
    archiveStream << config.toTomlString() << "\n";
    archiveStream << "---FILES---\n";

    // Gather all source files
    try {
        for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file()) {
                std::string relPath = fs::relative(entry.path(), projectDir).generic_string();
                std::string content = readFileContents(entry.path());
                archiveStream << "FILE:" << relPath << ":" << content.size() << "\n";
                archiveStream << content << "\n";
            }
        }
    } catch (const std::exception& ex) {
        result.errorMessage = "Failed to bundle source files: " + std::string(ex.what());
        return result;
    }

    // If README.md exists, include it
    fs::path readmePath = fs::path(projectDir) / "README.md";
    if (fs::exists(readmePath)) {
        std::string rmContent = readFileContents(readmePath);
        archiveStream << "FILE:README.md:" << rmContent.size() << "\n";
        archiveStream << rmContent << "\n";
    }

    std::string archiveBytes = archiveStream.str();
    {
        std::ofstream out(archivePath, std::ios::binary);
        if (!out.is_open()) {
            result.errorMessage = "Failed to create archive file: " + archivePath.string();
            return result;
        }
        out.write(archiveBytes.data(), archiveBytes.size());
    }

    result.packageArtifact = archivePath.string();
    result.archiveSizeBytes = archiveBytes.size();
    result.sha256Checksum = sp::rt::crypto::sha256(archiveBytes);

    // Also cache package in local offline cache for instant access
    std::string cacheErr;
    PackageCache::storePackage(pkg.name, pkg.version, projectDir, result.sha256Checksum, cacheErr);

    // 4. Token & upload handling
    if (options.dryRun) {
        result.success = true;
        result.registryResponse = "Dry run mode active: validation succeeded, archive created at " +
                                  archivePath.string() + ", checksum " + result.sha256Checksum;
        return result;
    }

    std::string token = resolveApiToken(options);
    if (token.empty()) {
        result.errorMessage = "Publishing requires an API token. Authenticate via 'spm login' or supply --token <token> / set SP_REGISTRY_TOKEN.";
        return result;
    }

    // Execute publication HTTP requests — Phase 4 slice A.
    //
    // 1. POST metadata to /api/v1/publish (Bearer). The registry stores
    //    the checksum as claimed; nothing is verified yet.
    // 2. POST the tarball bytes (base64) to
    //    /api/v1/packages/{name}/{version}/tarball (Bearer). Only then
    //    does the registry verify the checksum against real bytes
    //    (verified:true). An https:// registry URL works via the runtime
    //    HTTP client's TLS support (Phase 2) — the old "no TLS support"
    //    refusal is gone.
    // A failed/unreachable registry is a FAILURE. The old code treated
    // statusCode == 0 or >= 500 as success ("staged offline"); that
    // misled users into believing a package was published when it was not.
    HttpRegistryClient registryClient(options.registryUrl);
    RegistryPackageInfo info;
    info.name = pkg.name;
    info.version = pkg.version;
    info.description = pkg.description;
    info.license = "MIT";
    info.checksum = result.sha256Checksum;
    info.archiveSizeBytes = result.archiveSizeBytes;

    std::string metadataResponse, publishErr;
    if (!registryClient.publishMetadata(info, token, metadataResponse, publishErr)) {
        result.errorMessage = "Registry publish failed: " + publishErr +
                              " The package was validated and archived locally only; nothing was published.";
        return result;
    }

    // Upload the tarball so the registry can verify the checksum against
    // real bytes. One retry on transport failure only (not on 4xx/5xx).
    std::string uploadErr;
    bool uploaded = registryClient.uploadTarball(pkg.name, pkg.version, archiveBytes, token, uploadErr);
    if (!uploaded && uploadErr.find("unreachable") != std::string::npos) {
        uploadErr.clear();
        uploaded = registryClient.uploadTarball(pkg.name, pkg.version, archiveBytes, token, uploadErr);
    }
    if (!uploaded) {
        result.errorMessage = "Package metadata was published but the tarball upload failed: " + uploadErr +
            " The package stays unverified (verified:false) until the tarball is uploaded. "
            "Upload it manually: base64-encode " + archivePath.string() + " and POST it to " +
            registryClient.baseUrl() + "/api/v1/packages/" + pkg.name + "/" + pkg.version +
            "/tarball with your Bearer token. (Re-running 'spm publish' will fail: the registry "
            "rejects a duplicate version as immutable.)";
        return result;
    }

    result.success = true;
    // The metadata response alone still says verified:false (it was
    // written before the tarball arrived); report the upload outcome
    // too so the printed Status is not misleading.
    result.registryResponse = metadataResponse +
        "\nTarball uploaded and verified against the published checksum (verified:true).";
    return result;
}

} // namespace sp::spm
