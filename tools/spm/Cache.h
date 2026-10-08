#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace sp::spm {

namespace fs = std::filesystem;

class PackageCache {
public:
    // Determine the root cache directory:
    // 1. SP_CACHE_DIR environment variable
    // 2. $SP_HOME/cache
    // 3. ~/.sp/cache (%USERPROFILE%\.sp\cache on Windows, $HOME/.sp/cache on POSIX)
    static fs::path getCacheRootDir();

    // Directory where downloaded packages are stored (~/.sp/cache/packages/)
    static fs::path getPackagesRootDir();

    // Specific directory for a given package and version (~/.sp/cache/packages/<name>/<version>/)
    static fs::path getPackageDir(const std::string& name, const std::string& version);

    // Check if a package at a specific version is cached
    static bool isCached(const std::string& name, const std::string& version);

    // Store a package into local cache
    static bool storePackage(const std::string& name,
                             const std::string& version,
                             const fs::path& sourceDir,
                             const std::string& checksum,
                             std::string& errorMsg);

    // Verify stored package against an expected SHA-256 checksum
    static bool verifyChecksum(const std::string& name,
                               const std::string& version,
                               const std::string& expectedChecksum);

    // Read recorded checksum from cached package
    static std::string getCachedChecksum(const std::string& name, const std::string& version);

    // List all cached packages (returns pairs of {name, version})
    static std::vector<std::pair<std::string, std::string>> listCachedPackages();

    // Compute total disk size used by cache in bytes
    static size_t getCacheSizeBytes();

    // Clean all cached packages
    static bool clean(std::string& errorMsg);
};

} // namespace sp::spm
