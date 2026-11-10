// PackageArchive.cpp — Phase 4 slice A (spm / package ecosystem)
#include "PackageArchive.h"

#include <fstream>

namespace sp::spm {

namespace {

// Read one \n-terminated line starting at pos; advances pos past the \n.
// Returns false when no complete line remains.
bool readLine(const std::string& bytes, size_t& pos, std::string& line) {
    if (pos >= bytes.size()) return false;
    size_t nl = bytes.find('\n', pos);
    if (nl == std::string::npos) return false;
    line = bytes.substr(pos, nl - pos);
    pos = nl + 1;
    return true;
}

// Reject paths that would escape the destination directory.
bool isSafeRelPath(const std::string& rel) {
    if (rel.empty()) return false;
    if (rel[0] == '/' || rel[0] == '\\') return false;
    if (rel.find("..") != std::string::npos) return false;
    if (rel.find('\0') != std::string::npos) return false;
    return true;
}

} // namespace

std::string PackageArchive::archiveFileName(const std::string& name, const std::string& version) {
    return name + "-" + version + ".tar.gz";
}

bool PackageArchive::parse(const std::string& bytes, PackageArchiveData& out, std::string& err) {
    size_t pos = 0;
    std::string line;
    if (!readLine(bytes, pos, line) || line != "SP_PACKAGE_ARCHIVE_V1") {
        err = "not an SP package archive (missing SP_PACKAGE_ARCHIVE_V1 header)";
        return false;
    }

    // Header key=value lines until ---MANIFEST---
    bool sawManifest = false;
    while (readLine(bytes, pos, line)) {
        if (line == "---MANIFEST---") {
            sawManifest = true;
            break;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos || eq == 0) {
            err = "malformed archive header line: '" + line + "'";
            return false;
        }
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        if (key == "name") out.name = value;
        else if (key == "version") out.version = value;
        else if (key == "edition") out.edition = value;
        else if (key == "entry") out.entry = value;
        else if (key == "type") out.type = value;
        // Unknown header keys are ignored (forward compatibility).
    }
    if (!sawManifest) {
        err = "truncated archive: missing ---MANIFEST--- section";
        return false;
    }
    if (out.name.empty() || out.version.empty()) {
        err = "archive header is missing name/version";
        return false;
    }

    // Manifest: raw lines until ---FILES---
    bool sawFiles = false;
    std::string manifest;
    while (readLine(bytes, pos, line)) {
        if (line == "---FILES---") {
            sawFiles = true;
            break;
        }
        manifest += line;
        manifest += "\n";
    }
    if (!sawFiles) {
        err = "truncated archive: missing ---FILES--- section";
        return false;
    }
    out.manifestToml = manifest;

    // File entries: FILE:<relpath>:<size> then exactly <size> bytes + one '\n'.
    while (pos < bytes.size()) {
        if (!readLine(bytes, pos, line)) {
            err = "truncated archive: incomplete FILE entry header";
            return false;
        }
        if (line.empty()) continue; // tolerate a trailing blank line
        if (line.rfind("FILE:", 0) != 0) {
            err = "malformed archive: expected FILE: entry, got '" + line.substr(0, 40) + "'";
            return false;
        }
        size_t lastColon = line.rfind(':');
        if (lastColon <= 5) {
            err = "malformed FILE entry header: '" + line + "'";
            return false;
        }
        std::string rel = line.substr(5, lastColon - 5);
        std::string sizeStr = line.substr(lastColon + 1);
        size_t size = 0;
        try {
            size_t used = 0;
            size = std::stoul(sizeStr, &used);
            if (used != sizeStr.size()) throw std::invalid_argument("trailing chars");
        } catch (const std::exception&) {
            err = "malformed FILE entry size: '" + sizeStr + "'";
            return false;
        }
        if (!isSafeRelPath(rel)) {
            err = "unsafe path in archive: '" + rel + "'";
            return false;
        }
        if (pos + size > bytes.size()) {
            err = "truncated archive: file '" + rel + "' declares " + std::to_string(size) +
                  " bytes but only " + std::to_string(bytes.size() - pos) + " remain";
            return false;
        }
        ArchiveFileEntry entry;
        entry.relPath = rel;
        entry.bytes = bytes.substr(pos, size);
        pos += size;
        // The writer appends one '\n' after each file's contents.
        if (pos < bytes.size() && bytes[pos] == '\n') pos += 1;
        out.files.push_back(std::move(entry));
    }
    return true;
}

bool PackageArchive::extract(const PackageArchiveData& data, const fs::path& destDir,
                             const std::string& checksum, std::string& err) {
    std::error_code ec;
    fs::create_directories(destDir, ec);
    if (ec) {
        err = "cannot create vendor directory '" + destDir.string() + "': " + ec.message();
        return false;
    }

    for (const auto& file : data.files) {
        fs::path target;
        if (!data.entry.empty() && file.relPath == data.entry) {
            // Entry file -> mod.sp (so `import <name>;` resolves).
            target = destDir / "mod.sp";
        } else if (file.relPath.rfind("src/", 0) == 0) {
            // src/foo.sp -> foo.sp (so `import <name>.foo;` resolves).
            target = destDir / file.relPath.substr(4);
        } else {
            target = destDir / file.relPath;
        }
        if (target.has_parent_path()) {
            fs::create_directories(target.parent_path(), ec);
            if (ec) {
                err = "cannot create directory for '" + target.string() + "': " + ec.message();
                return false;
            }
        }
        std::ofstream out(target, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            err = "cannot write vendored file '" + target.string() + "'";
            return false;
        }
        out.write(file.bytes.data(), static_cast<std::streamsize>(file.bytes.size()));
        if (!out) {
            err = "failed writing vendored file '" + target.string() + "'";
            return false;
        }
    }

    // Marker so ensureVendored() can tell whether this tree still matches
    // the lockfile entry without re-extracting.
    if (!checksum.empty()) {
        std::ofstream marker(destDir / ".sp-checksum", std::ios::trunc);
        if (marker.is_open()) marker << checksum;
    }
    return true;
}

} // namespace sp::spm
