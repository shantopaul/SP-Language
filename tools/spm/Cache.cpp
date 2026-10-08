#include "Cache.h"
#include "sp/runtime/Crypto.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace sp::spm {

fs::path PackageCache::getCacheRootDir() {
    const char* envCache = std::getenv("SP_CACHE_DIR");
    if (envCache && *envCache) {
        return fs::path(envCache);
    }

    const char* envHome = std::getenv("SP_HOME");
    if (envHome && *envHome) {
        return fs::path(envHome) / "cache";
    }

#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        return fs::path(userProfile) / ".sp" / "cache";
    }
#else
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return fs::path(home) / ".sp" / "cache";
    }
#endif

    return fs::temp_directory_path() / ".sp_cache";
}

fs::path PackageCache::getPackagesRootDir() {
    return getCacheRootDir() / "packages";
}

fs::path PackageCache::getPackageDir(const std::string& name, const std::string& version) {
    return getPackagesRootDir() / name / version;
}

bool PackageCache::isCached(const std::string& name, const std::string& version) {
    fs::path dir = getPackageDir(name, version);
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return false;
    }
    // Must contain sp.toml to be considered valid
    return fs::exists(dir / "sp.toml");
}

bool PackageCache::storePackage(const std::string& name,
                               const std::string& version,
                               const fs::path& sourceDir,
                               const std::string& checksum,
                               std::string& errorMsg) {
    if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
        errorMsg = "Source package directory does not exist: " + sourceDir.string();
        return false;
    }

    fs::path targetDir = getPackageDir(name, version);
    std::error_code ec;
    fs::create_directories(targetDir, ec);
    if (ec) {
        errorMsg = "Failed to create cache directory: " + ec.message();
        return false;
    }

    // Copy contents of sourceDir to targetDir
    try {
        fs::copy(sourceDir, targetDir,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            errorMsg = "Failed to copy package contents to cache: " + ec.message();
            return false;
        }

        // Write checksum file
        if (!checksum.empty()) {
            std::ofstream cksOut(targetDir / ".sp-checksum");
            if (cksOut.is_open()) {
                cksOut << checksum;
            }
        }
    } catch (const std::exception& ex) {
        errorMsg = "Exception caching package: " + std::string(ex.what());
        return false;
    }

    return true;
}

std::string PackageCache::getCachedChecksum(const std::string& name, const std::string& version) {
    fs::path cksPath = getPackageDir(name, version) / ".sp-checksum";
    if (!fs::exists(cksPath)) return "";
    std::ifstream in(cksPath);
    if (!in.is_open()) return "";
    std::string line;
    std::getline(in, line);
    return line;
}

bool PackageCache::verifyChecksum(const std::string& name,
                                 const std::string& version,
                                 const std::string& expectedChecksum) {
    if (expectedChecksum.empty()) return true;
    std::string cached = getCachedChecksum(name, version);
    if (cached.empty()) return false;
    return cached == expectedChecksum;
}

std::vector<std::pair<std::string, std::string>> PackageCache::listCachedPackages() {
    std::vector<std::pair<std::string, std::string>> list;
    fs::path root = getPackagesRootDir();
    if (!fs::exists(root) || !fs::is_directory(root)) return list;

    try {
        for (const auto& pkgEntry : fs::directory_iterator(root)) {
            if (pkgEntry.is_directory()) {
                std::string pkgName = pkgEntry.path().filename().string();
                for (const auto& verEntry : fs::directory_iterator(pkgEntry.path())) {
                    if (verEntry.is_directory() && fs::exists(verEntry.path() / "sp.toml")) {
                        std::string verName = verEntry.path().filename().string();
                        list.emplace_back(pkgName, verName);
                    }
                }
            }
        }
    } catch (...) {}

    return list;
}

size_t PackageCache::getCacheSizeBytes() {
    fs::path root = getPackagesRootDir();
    if (!fs::exists(root)) return 0;

    size_t total = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (entry.is_regular_file()) {
                std::error_code ec;
                total += fs::file_size(entry.path(), ec);
            }
        }
    } catch (...) {}
    return total;
}

bool PackageCache::clean(std::string& errorMsg) {
    fs::path root = getPackagesRootDir();
    if (!fs::exists(root)) return true;

    std::error_code ec;
    fs::remove_all(root, ec);
    if (ec) {
        errorMsg = "Failed to clean cache directory: " + ec.message();
        return false;
    }
    return true;
}

} // namespace sp::spm
