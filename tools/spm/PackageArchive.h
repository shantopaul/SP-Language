// PackageArchive.h — Phase 4 slice A (spm / package ecosystem)
//
// Parser/extractor for the SP_PACKAGE_ARCHIVE_V1 format written by
// PackagePublisher::publish (tools/spm/Publish.cpp). The format is a
// plain-text container (NOT gzip despite the .tar.gz filename):
//
//   SP_PACKAGE_ARCHIVE_V1\n
//   name=<name>\n version=<version>\n edition=<edition>\n
//   entry=<entry>\n type=<type>\n
//   ---MANIFEST---\n
//   <sp.toml text>\n
//   ---FILES---\n
//   FILE:<relpath>:<size>\n<exactly size raw bytes>\n   ... (repeat per file)
//
// Files are the package's src/ tree (rel paths like src/lib.sp) plus an
// optional README.md. Parsing is strict: a bad header, a truncated file
// entry, or a size mismatch is an error, never a silent partial extract.
//
// Vendor layout (extract): the package entry file (sp.toml `entry`,
// usually src/lib.sp) becomes <dest>/mod.sp so `import <name>;` resolves;
// every other src/<rest> becomes <dest>/<rest> so `import <name>.foo;`
// resolves; non-src files (README.md) keep their relative path.
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sp::spm {

namespace fs = std::filesystem;

struct ArchiveFileEntry {
    std::string relPath; // as stored, e.g. "src/lib.sp"
    std::string bytes;   // exact file contents
};

struct PackageArchiveData {
    std::string name;
    std::string version;
    std::string edition;
    std::string entry; // e.g. "src/lib.sp"
    std::string type;  // "lib" or "bin"
    std::string manifestToml; // raw sp.toml text from the ---MANIFEST--- section
    std::vector<ArchiveFileEntry> files;
};

class PackageArchive {
public:
    // Parse archive bytes. Returns false + err on any malformed input.
    static bool parse(const std::string& bytes, PackageArchiveData& out, std::string& err);

    // Extract with the vendor layout described above into destDir
    // (created if needed; existing files are overwritten). Writes a
    // `.sp-checksum` marker file holding `checksum` so callers can tell
    // later whether the vendored tree still matches a lockfile entry.
    static bool extract(const PackageArchiveData& data, const fs::path& destDir,
                        const std::string& checksum, std::string& err);

    // "<name>-<version>.tar.gz" — the on-disk archive file name used by
    // both the publisher (target/packages/) and the package cache.
    static std::string archiveFileName(const std::string& name, const std::string& version);
};

} // namespace sp::spm
