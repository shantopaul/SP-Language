// RegistryEcosystemTest.cpp — Phase 4 slice A (spm / package ecosystem)
//
// Tests for the real-registry wiring: SP_PACKAGE_ARCHIVE_V1 parse/extract,
// RegistryClient JSON parsing + URL resolution, RegistryProvider
// (registry-first resolution, content checksums, offline cache fallback,
// loud refusal to silently downgrade), cache archive-hash verification,
// and BuildOrchestrator::install vendoring against a stub registry
// client (no live server in unit tests; the live round-trip is proven
// by the Phase 4 slice A verification run instead).
#include <gtest/gtest.h>
#include "spm/RegistryClient.h"
#include "spm/PackageArchive.h"
#include "spm/DependencyResolver.h"
#include "spm/BuildOrchestrator.h"
#include "spm/Cache.h"
#include "spm/Lockfile.h"
#include "sp/runtime/Crypto.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <chrono>

namespace fs = std::filesystem;
using namespace sp::spm;

namespace {

// Build minimal but valid SP_PACKAGE_ARCHIVE_V1 bytes in memory.
std::string makeArchive(const std::string& name, const std::string& version,
                        const std::string& libBody) {
    std::string manifest = "[package]\nname = \"" + name + "\"\nversion = \"" + version +
        "\"\ndescription = \"test lib\"\nedition = \"2026\"\nentry = \"src/lib.sp\"\ntype = \"lib\"\n\n[dependencies]\n";
    std::string utilBody = "module " + name + ".util;\n\npublic fn util() -> string {\n    return \"util\";\n}\n";
    std::string readme = "# " + name + "\n";
    std::ostringstream ss;
    ss << "SP_PACKAGE_ARCHIVE_V1\n";
    ss << "name=" << name << "\n";
    ss << "version=" << version << "\n";
    ss << "edition=2026\n";
    ss << "entry=src/lib.sp\n";
    ss << "type=lib\n";
    ss << "---MANIFEST---\n";
    ss << manifest << "\n";
    ss << "---FILES---\n";
    ss << "FILE:src/lib.sp:" << libBody.size() << "\n" << libBody << "\n";
    ss << "FILE:src/util.sp:" << utilBody.size() << "\n" << utilBody << "\n";
    ss << "FILE:README.md:" << readme.size() << "\n" << readme << "\n";
    return ss.str();
}

std::string readFileBytes(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// In-memory registry stand-in implementing the same contract as
// HttpRegistryClient (including SHA-256 verification on download).
class StubRegistryClient : public IRegistryClient {
public:
    struct Entry {
        RegistryPackageInfo info;
        std::string archiveBytes;
    };

    void addPackage(const std::string& name, const std::string& version,
                    const std::string& libBody) {
        std::string bytes = makeArchive(name, version, libBody);
        Entry e;
        e.info.name = name;
        e.info.version = version;
        e.info.description = "test lib";
        e.info.license = "MIT";
        e.info.checksum = sp::rt::crypto::sha256(bytes);
        e.info.archiveSizeBytes = bytes.size();
        e.info.tarballStored = true;
        e.info.downloadUrl = "/api/v1/packages/" + name + "/" + version + "/download";
        e.archiveBytes = bytes;
        m_entries[name + "@" + version] = std::move(e);
    }

    // Fail the version list (transport error) to simulate a down registry.
    bool failList{false};
    // Fail downloads for these versions (e.g. tarball never uploaded).
    std::vector<std::string> failDownloadVersions;

    std::string baseUrl() const override { return "http://stub-registry.invalid"; }

    bool listPackages(std::vector<RegistryPackageInfo>& out, std::string& err) override {
        if (failList) {
            err = "GET /api/v1/packages: registry unreachable (connection failed)";
            return false;
        }
        out.clear();
        for (const auto& [key, e] : m_entries) {
            (void)key;
            out.push_back(e.info);
        }
        return true;
    }

    bool getMetadata(const std::string& name, const std::string& version,
                     RegistryPackageInfo& out, std::string& err) override {
        auto it = m_entries.find(name + "@" + version);
        if (it == m_entries.end()) {
            err = "package '" + name + "@" + version + "' not found in registry at " + baseUrl();
            return false;
        }
        out = it->second.info;
        return true;
    }

    bool download(const std::string& name, const std::string& version,
                  std::string& outBytes, std::string& outChecksum, std::string& err) override {
        auto it = m_entries.find(name + "@" + version);
        if (it == m_entries.end()) {
            err = "no tarball stored for '" + name + "@" + version + "' in the registry";
            return false;
        }
        for (const auto& v : failDownloadVersions) {
            if (v == version) {
                err = "package '" + name + "@" + version +
                      "' has no verified tarball in the registry (published metadata only)";
                return false;
            }
        }
        std::string computed = sp::rt::crypto::sha256(it->second.archiveBytes);
        if (computed != it->second.info.checksum) {
            err = "downloaded tarball fails SHA-256 verification; refusing to install";
            return false;
        }
        outBytes = it->second.archiveBytes;
        outChecksum = computed;
        return true;
    }

    bool publishMetadata(const RegistryPackageInfo& info, const std::string& token,
                         std::string& outResponse, std::string& err) override {
        (void)info;
        (void)token;
        (void)outResponse;
        err = "stub does not implement publish";
        return false;
    }

    bool uploadTarball(const std::string& name, const std::string& version,
                       const std::string& archiveBytes, const std::string& token,
                       std::string& err) override {
        (void)name;
        (void)version;
        (void)archiveBytes;
        (void)token;
        err = "stub does not implement upload";
        return false;
    }

private:
    std::map<std::string, Entry> m_entries;
};

class RegistryEcosystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tempDir = fs::temp_directory_path() /
            ("spm_p4a_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
        fs::create_directories(m_tempDir, ec);
        // Isolate the package cache (and the registry URL) per test.
        const char* oldCache = std::getenv("SP_CACHE_DIR");
        m_oldCacheDir = oldCache ? oldCache : "";
        m_cacheDir = m_tempDir / "cache";
        fs::create_directories(m_cacheDir, ec);
        setenv("SP_CACHE_DIR", m_cacheDir.string().c_str(), 1);
        const char* oldUrl = std::getenv("SP_REGISTRY_URL");
        m_oldRegistryUrl = oldUrl ? oldUrl : "";
        unsetenv("SP_REGISTRY_URL");
    }

    void TearDown() override {
        if (m_oldCacheDir.empty()) {
            unsetenv("SP_CACHE_DIR");
        } else {
            setenv("SP_CACHE_DIR", m_oldCacheDir.c_str(), 1);
        }
        if (m_oldRegistryUrl.empty()) {
            unsetenv("SP_REGISTRY_URL");
        } else {
            setenv("SP_REGISTRY_URL", m_oldRegistryUrl.c_str(), 1);
        }
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    fs::path m_tempDir;
    fs::path m_cacheDir;
    std::string m_oldCacheDir;
    std::string m_oldRegistryUrl;
};

} // namespace

// --- PackageArchive ---------------------------------------------------------

TEST_F(RegistryEcosystemTest, ArchiveRoundTripPreservesFilesAndVendorLayout) {
    std::string libBody = "module greet;\n\npublic fn greet(string target) -> string {\n    return \"Hello, \" + target + \"!\";\n}\n";
    std::string bytes = makeArchive("greet", "0.2.0", libBody);

    PackageArchiveData data;
    std::string err;
    ASSERT_TRUE(PackageArchive::parse(bytes, data, err)) << err;
    EXPECT_EQ(data.name, "greet");
    EXPECT_EQ(data.version, "0.2.0");
    EXPECT_EQ(data.entry, "src/lib.sp");
    ASSERT_EQ(data.files.size(), 3u);
    EXPECT_NE(data.manifestToml.find("name = \"greet\""), std::string::npos);

    fs::path destDir = m_tempDir / "vendor" / "greet";
    std::string checksum = sp::rt::crypto::sha256(bytes);
    ASSERT_TRUE(PackageArchive::extract(data, destDir, checksum, err)) << err;

    // Vendor layout: entry -> mod.sp, other src/*.sp -> *.sp.
    EXPECT_EQ(readFileBytes(destDir / "mod.sp"), libBody);
    EXPECT_NE(readFileBytes(destDir / "util.sp").find("public fn util()"), std::string::npos);
    EXPECT_TRUE(fs::exists(destDir / "README.md"));
    EXPECT_EQ(readFileBytes(destDir / ".sp-checksum"), checksum);
}

TEST_F(RegistryEcosystemTest, ArchiveParseRejectsGarbageAndTruncation) {
    PackageArchiveData data;
    std::string err;
    EXPECT_FALSE(PackageArchive::parse("hello, world", data, err));
    EXPECT_FALSE(PackageArchive::parse("SP_PACKAGE_ARCHIVE_V1\nname=greet\n", data, err));

    std::string good = makeArchive("greet", "0.1.0", "module greet;\n");
    // Truncate mid-file-body.
    std::string truncated = good.substr(0, good.size() - 20);
    EXPECT_FALSE(PackageArchive::parse(truncated, data, err));
    // Corrupt a FILE size.
    std::string badSize = good;
    size_t pos = badSize.find("FILE:src/lib.sp:");
    ASSERT_NE(pos, std::string::npos);
    badSize.replace(pos, std::string("FILE:src/lib.sp:99999").size(), "FILE:src/lib.sp:99999");
    EXPECT_FALSE(PackageArchive::parse(badSize, data, err));
}

// --- RegistryClient (pure parsing + URL resolution) --------------------------

TEST_F(RegistryEcosystemTest, ClientParsesListAndMetadataJson) {
    std::string listJson =
        R"({"packages":[{"name":"greet","version":"0.1.0","description":"A lib","license":"MIT","checksum":"aaa","archiveSizeBytes":386,"tarballStored":false,"downloadUrl":"/api/v1/packages/greet/0.1.0/download"},{"name":"greet","version":"0.2.0","description":"A lib","license":"MIT","checksum":"bbb","archiveSizeBytes":393,"tarballStored":true,"downloadUrl":"/api/v1/packages/greet/0.2.0/download"}],"count":2})";
    std::vector<RegistryPackageInfo> pkgs;
    std::string err;
    ASSERT_TRUE(HttpRegistryClient::parsePackageList(listJson, pkgs, err)) << err;
    ASSERT_EQ(pkgs.size(), 2u);
    EXPECT_EQ(pkgs[0].name, "greet");
    EXPECT_EQ(pkgs[0].version, "0.1.0");
    EXPECT_FALSE(pkgs[0].tarballStored);
    EXPECT_TRUE(pkgs[1].tarballStored);
    EXPECT_EQ(pkgs[1].checksum, "bbb");

    std::string metaJson =
        R"({"name":"greet","version":"0.2.0","description":"A lib","license":"MIT","checksum":"bbb","archiveSizeBytes":393,"tarballStored":true,"downloadUrl":"/api/v1/packages/greet/0.2.0/download"})";
    RegistryPackageInfo info;
    ASSERT_TRUE(HttpRegistryClient::parsePackageObject(metaJson, info, err)) << err;
    EXPECT_EQ(info.version, "0.2.0");
    EXPECT_EQ(info.archiveSizeBytes, 393u);

    EXPECT_FALSE(HttpRegistryClient::parsePackageList("not json", pkgs, err));
    EXPECT_FALSE(HttpRegistryClient::parsePackageObject(R"({"version":"1.0.0"})", info, err));
}

TEST_F(RegistryEcosystemTest, ClientResolvesBaseUrlExplicitThenEnvThenDefault) {
    EXPECT_EQ(HttpRegistryClient::resolveBaseUrl("https://example.invalid/"), "https://example.invalid");
    EXPECT_EQ(HttpRegistryClient::defaultBaseUrl(), "http://127.0.0.1:8090");
    EXPECT_EQ(HttpRegistryClient::resolveBaseUrl(""), "http://127.0.0.1:8090");
    setenv("SP_REGISTRY_URL", "http://127.0.0.1:9999/", 1);
    EXPECT_EQ(HttpRegistryClient::resolveBaseUrl(""), "http://127.0.0.1:9999");
    EXPECT_EQ(HttpRegistryClient::resolveBaseUrl("http://other.invalid"), "http://other.invalid");
}

// --- RegistryProvider --------------------------------------------------------

TEST_F(RegistryEcosystemTest, ProviderPicksHighestMatchingVersionWithContentChecksum) {
    auto client = std::make_shared<StubRegistryClient>();
    client->addPackage("greet", "0.1.0", "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n");
    client->addPackage("greet", "0.2.0", "module greet;\npublic fn greet(string t) -> string { return \"v2\"; }\n");
    auto provider = std::make_shared<RegistryProvider>(client);

    // "*" picks the highest version, and the lock checksum is the real
    // tarball SHA-256, not the legacy metadata hash.
    {
        ProjectConfig root;
        root.getPackage().name = "app";
        root.getPackage().version = "0.1.0";
        Dependency dep;
        dep.name = "greet";
        dep.versionReq = "*";
        root.addDependency(dep);
        DependencyResolver resolver(provider);
        auto res = resolver.resolve(root, ".", false);
        ASSERT_TRUE(res.success) << res.errorMessage;
        ASSERT_EQ(res.resolvedPackages.size(), 1u);
        EXPECT_EQ(res.resolvedPackages[0].version.toString(), "0.2.0");
        std::string expected = sp::rt::crypto::sha256(
            makeArchive("greet", "0.2.0", "module greet;\npublic fn greet(string t) -> string { return \"v2\"; }\n"));
        EXPECT_EQ(res.resolvedPackages[0].checksum, expected);
        EXPECT_NE(res.resolvedPackages[0].checksum,
                  DependencyResolver::computeChecksum("greet", *Version::parse("0.2.0"), "registry"));
    }

    // "^0.1.0" is a 0-major caret range: it matches 0.1.x only, NOT 0.2.0.
    {
        ProjectConfig root;
        root.getPackage().name = "app";
        root.getPackage().version = "0.1.0";
        Dependency dep;
        dep.name = "greet";
        dep.versionReq = "^0.1.0";
        root.addDependency(dep);
        DependencyResolver resolver(provider);
        auto res = resolver.resolve(root, ".", false);
        ASSERT_TRUE(res.success) << res.errorMessage;
        ASSERT_EQ(res.resolvedPackages.size(), 1u);
        EXPECT_EQ(res.resolvedPackages[0].version.toString(), "0.1.0");
    }

    // Unsatisfiable constraint fails loudly, never silently installs.
    {
        ProjectConfig root;
        root.getPackage().name = "app";
        root.getPackage().version = "0.1.0";
        Dependency dep;
        dep.name = "greet";
        dep.versionReq = "^9.0.0";
        root.addDependency(dep);
        DependencyResolver resolver(provider);
        auto res = resolver.resolve(root, ".", false);
        EXPECT_FALSE(res.success);
        EXPECT_NE(res.errorMessage.find("No compatible version"), std::string::npos);
    }
}

TEST_F(RegistryEcosystemTest, ProviderRefusesSilentDowngradeWhenNewestUnfetchable) {
    auto client = std::make_shared<StubRegistryClient>();
    client->addPackage("greet", "0.1.0", "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n");
    client->addPackage("greet", "0.2.0", "module greet;\npublic fn greet(string t) -> string { return \"v2\"; }\n");
    client->failDownloadVersions = {"0.2.0"}; // metadata exists, tarball cannot be fetched
    auto provider = std::make_shared<RegistryProvider>(client);

    ProjectConfig root;
    root.getPackage().name = "app";
    root.getPackage().version = "0.1.0";
    Dependency dep;
    dep.name = "greet";
    dep.versionReq = "*";
    root.addDependency(dep);
    DependencyResolver resolver(provider);
    auto res = resolver.resolve(root, ".", false);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.errorMessage.find("Refusing to install an older version"), std::string::npos);
}

TEST_F(RegistryEcosystemTest, ProviderFallsBackToCacheWhenRegistryIsDown) {
    // Pre-populate the cache with greet 0.1.0 (as a previous install would).
    std::string libBody = "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n";
    std::string bytes = makeArchive("greet", "0.1.0", libBody);
    std::string checksum = sp::rt::crypto::sha256(bytes);
    std::string err;
    ASSERT_TRUE(PackageCache::storeArchive("greet", "0.1.0", bytes, checksum, err)) << err;

    auto client = std::make_shared<StubRegistryClient>();
    client->failList = true; // registry unreachable
    auto provider = std::make_shared<RegistryProvider>(client);

    ProjectConfig root;
    root.getPackage().name = "app";
    root.getPackage().version = "0.1.0";
    Dependency dep;
    dep.name = "greet";
    dep.versionReq = "^0.1.0";
    root.addDependency(dep);
    DependencyResolver resolver(provider);
    auto res = resolver.resolve(root, ".", false);
    ASSERT_TRUE(res.success) << res.errorMessage;
    ASSERT_EQ(res.resolvedPackages.size(), 1u);
    EXPECT_EQ(res.resolvedPackages[0].version.toString(), "0.1.0");
    // Even offline, the lock pins the cached archive's content hash —
    // not the legacy metadata hash — so vendoring + verify work.
    EXPECT_EQ(res.resolvedPackages[0].checksum, checksum);
    EXPECT_TRUE(provider->registryUnavailable());
}

// --- Cache ------------------------------------------------------------------

TEST_F(RegistryEcosystemTest, CacheArchiveHashDetectsTampering) {
    std::string libBody = "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n";
    std::string bytes = makeArchive("greet", "0.1.0", libBody);
    std::string checksum = sp::rt::crypto::sha256(bytes);
    std::string err;
    ASSERT_TRUE(PackageCache::storeArchive("greet", "0.1.0", bytes, checksum, err)) << err;
    EXPECT_TRUE(PackageCache::verifyChecksum("greet", "0.1.0", checksum));

    // Tamper with one byte of the cached archive file.
    fs::path archivePath = PackageCache::getCachedArchivePath("greet", "0.1.0");
    ASSERT_TRUE(fs::exists(archivePath));
    std::string tampered = readFileBytes(archivePath);
    ASSERT_FALSE(tampered.empty());
    tampered[tampered.size() / 2] ^= 0x01;
    {
        std::ofstream out(archivePath, std::ios::binary | std::ios::trunc);
        out.write(tampered.data(), static_cast<std::streamsize>(tampered.size()));
    }
    EXPECT_FALSE(PackageCache::verifyChecksum("greet", "0.1.0", checksum));
    // The recorded .sp-checksum string is unchanged by the tamper — the
    // content hash (not the record) is what catches it.
    EXPECT_EQ(PackageCache::getCachedChecksum("greet", "0.1.0"), checksum);
}

// --- install() end-to-end against the stub registry --------------------------

TEST_F(RegistryEcosystemTest, InstallVendorsFromRegistryAndVerifyPasses) {
    std::string err;
    fs::path projDir = m_tempDir / "hello";
    ASSERT_TRUE(scaffoldProject(projDir.string(), "hello", false, err)) << err;

    ProjectConfig cfg;
    ASSERT_TRUE(cfg.loadFromFile((projDir / "sp.toml").string()));
    Dependency dep;
    dep.name = "greet";
    dep.versionReq = "*";
    cfg.addDependency(dep);
    ASSERT_TRUE(cfg.saveToFile((projDir / "sp.toml").string()));

    auto client = std::make_shared<StubRegistryClient>();
    client->addPackage("greet", "0.1.0", "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n");
    client->addPackage("greet", "0.2.0", "module greet;\npublic fn greet(string t) -> string { return \"v2\"; }\n");

    auto res = BuildOrchestrator::install(projDir.string(), client, false, false);
    ASSERT_TRUE(res.success) << res.errorMessage;
    EXPECT_EQ(res.packagesInstalled, 1u);

    // sp.lock pins the real tarball SHA-256 of greet 0.2.0.
    Lockfile lock;
    ASSERT_TRUE(lock.loadFromFile((projDir / "sp.lock").string()));
    ASSERT_EQ(lock.getPackages().size(), 1u);
    EXPECT_EQ(lock.getPackages()[0].version, "0.2.0");
    std::string expected = sp::rt::crypto::sha256(
        makeArchive("greet", "0.2.0", "module greet;\npublic fn greet(string t) -> string { return \"v2\"; }\n"));
    EXPECT_EQ(lock.getPackages()[0].checksum, expected);

    // Vendored layout: import greet; resolves to mod.sp.
    fs::path modSp = projDir / ".spm" / "packages" / "greet" / "mod.sp";
    ASSERT_TRUE(fs::exists(modSp));
    EXPECT_NE(readFileBytes(modSp).find("return \"v2\""), std::string::npos);
    EXPECT_TRUE(fs::exists(projDir / ".spm" / "packages" / "greet" / "util.sp"));

    // verify() passes against the cached archive bytes.
    auto v = BuildOrchestrator::verify(projDir.string());
    EXPECT_TRUE(v.success) << v.errorMessage;
    EXPECT_EQ(v.packagesVerified, 1u);
}

TEST_F(RegistryEcosystemTest, InstallVendorsAgainOnFreshCheckout) {
    std::string err;
    fs::path projDir = m_tempDir / "hello";
    ASSERT_TRUE(scaffoldProject(projDir.string(), "hello", false, err)) << err;

    ProjectConfig cfg;
    ASSERT_TRUE(cfg.loadFromFile((projDir / "sp.toml").string()));
    Dependency dep;
    dep.name = "greet";
    dep.versionReq = "^0.1.0";
    cfg.addDependency(dep);
    ASSERT_TRUE(cfg.saveToFile((projDir / "sp.toml").string()));

    auto client = std::make_shared<StubRegistryClient>();
    client->addPackage("greet", "0.1.0", "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n");

    auto first = BuildOrchestrator::install(projDir.string(), client, false, false);
    ASSERT_TRUE(first.success) << first.errorMessage;
    ASSERT_TRUE(fs::exists(projDir / ".spm" / "packages" / "greet" / "mod.sp"));

    // Fresh checkout: lockfile present, vendored tree deleted.
    std::error_code ec;
    fs::remove_all(projDir / ".spm", ec);
    ASSERT_FALSE(fs::exists(projDir / ".spm" / "packages" / "greet" / "mod.sp"));

    // Second install must NOT no-op on the up-to-date lock: it re-vendors.
    auto second = BuildOrchestrator::install(projDir.string(), client, false, false);
    ASSERT_TRUE(second.success) << second.errorMessage;
    EXPECT_TRUE(fs::exists(projDir / ".spm" / "packages" / "greet" / "mod.sp"));
}

TEST_F(RegistryEcosystemTest, InstallRelocksLegacyChecksumWithoutChangingVersion) {
    std::string err;
    fs::path projDir = m_tempDir / "hello";
    ASSERT_TRUE(scaffoldProject(projDir.string(), "hello", false, err)) << err;

    ProjectConfig cfg;
    ASSERT_TRUE(cfg.loadFromFile((projDir / "sp.toml").string()));
    Dependency dep;
    dep.name = "greet";
    dep.versionReq = "^0.1.0";
    cfg.addDependency(dep);
    ASSERT_TRUE(cfg.saveToFile((projDir / "sp.toml").string()));

    // Legacy lockfile: metadata (FNV) checksum, version pinned at 0.1.0.
    auto ver = *Version::parse("0.1.0");
    std::string legacy = DependencyResolver::computeChecksum("greet", ver, "registry");
    Lockfile lock;
    lock.addPackage(LockedPackage{"greet", "0.1.0", "registry", legacy, {}});
    ASSERT_TRUE(lock.saveToFile((projDir / "sp.lock").string()));

    auto client = std::make_shared<StubRegistryClient>();
    client->addPackage("greet", "0.1.0", "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n");
    client->addPackage("greet", "0.2.0", "module greet;\npublic fn greet(string t) -> string { return \"v2\"; }\n");

    auto res = BuildOrchestrator::install(projDir.string(), client, false, false);
    ASSERT_TRUE(res.success) << res.errorMessage;

    Lockfile relocked;
    ASSERT_TRUE(relocked.loadFromFile((projDir / "sp.lock").string()));
    ASSERT_EQ(relocked.getPackages().size(), 1u);
    EXPECT_EQ(relocked.getPackages()[0].version, "0.1.0"); // version unchanged
    std::string expected = sp::rt::crypto::sha256(
        makeArchive("greet", "0.1.0", "module greet;\npublic fn greet(string t) -> string { return \"v1\"; }\n"));
    EXPECT_EQ(relocked.getPackages()[0].checksum, expected); // now a content hash
}

TEST_F(RegistryEcosystemTest, VerifyFailsLegacyLockfile) {
    std::string err;
    fs::path projDir = m_tempDir / "hello";
    ASSERT_TRUE(scaffoldProject(projDir.string(), "hello", false, err)) << err;

    ProjectConfig cfg;
    ASSERT_TRUE(cfg.loadFromFile((projDir / "sp.toml").string()));
    Dependency dep;
    dep.name = "greet";
    dep.versionReq = "^0.1.0";
    cfg.addDependency(dep);
    ASSERT_TRUE(cfg.saveToFile((projDir / "sp.toml").string()));

    auto ver = *Version::parse("0.1.0");
    Lockfile lock;
    lock.addPackage(LockedPackage{"greet", "0.1.0", "registry",
                                 DependencyResolver::computeChecksum("greet", ver, "registry"), {}});
    ASSERT_TRUE(lock.saveToFile((projDir / "sp.lock").string()));

    auto v = BuildOrchestrator::verify(projDir.string());
    EXPECT_FALSE(v.success);
    EXPECT_NE(v.errorMessage.find("legacy"), std::string::npos);
}
