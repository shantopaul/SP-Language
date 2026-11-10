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

    // Store downloaded archive bytes into the cache
    // (~/.sp/cache/packages/<name>/<version>/target/packages/<name>-<version>.tar.gz),
    // record `checksum` in .sp-checksum, and extract the archive's files
    // (sp.toml + src/) into the cache dir so getPackageConfig can read
    // the manifest without a network round-trip. `checksum` must be the
    // SHA-256 of `archiveBytes` (verified by the caller).
    static bool storeArchive(const std::string& name,
                             const std::string& version,
                             const std::string& archiveBytes,
                             const std::string& checksum,
                             std::string& errorMsg);

    // Path of the cached archive file for a package version (may not exist).
    static fs::path getCachedArchivePath(const std::string& name, const std::string& version);

    // Read the cached archive bytes. Returns false when no archive is cached.
    static bool readCachedArchive(const std::string& name, const std::string& version,
                                  std::string& outBytes);

    // Verify stored package against an expected SHA-256 checksum.
    // Phase 4 slice A: when an archive file is cached, its bytes are
    // hashed and compared (content check). Only when no archive exists
    // (legacy cache populated by a plain directory copy) does it fall
    // back to comparing the recorded .sp-checksum string — that fallback
    // detects a changed record, NOT tampered files, and callers that
    // need content assurance (verify/install) treat a missing archive
    // as unverifiable rather than verified.
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
