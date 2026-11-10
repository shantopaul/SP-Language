#include <gtest/gtest.h>
#include "spm/BuildOrchestrator.h"
#include "spm/ProjectConfig.h"
#include "spm/Publish.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace sp::spm;

class BuildOrchestratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tempDir = fs::temp_directory_path() / ("spm_orch_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
        fs::create_directories(m_tempDir, ec);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    fs::path m_tempDir;
};

TEST_F(BuildOrchestratorTest, BuildAndCleanProject) {
    std::string err;
    ASSERT_TRUE(scaffoldProject(m_tempDir.string(), "hello_app", false, err)) << err;

    BuildOptions opts;
    auto res = BuildOrchestrator::build(m_tempDir.string(), opts);
    ASSERT_TRUE(res.success) << res.errorMessage;
    EXPECT_TRUE(fs::exists(res.targetBinary));

    // Test run
    int exitCode = BuildOrchestrator::run(m_tempDir.string(), opts, {});
    EXPECT_EQ(exitCode, 0);

    // Test clean
    EXPECT_TRUE(BuildOrchestrator::clean(m_tempDir.string(), err));
    EXPECT_FALSE(fs::exists(m_tempDir / "target"));
}

TEST_F(BuildOrchestratorTest, PublishProjectValidationAndArtifact) {
    std::string err;
    ASSERT_TRUE(scaffoldProject(m_tempDir.string(), "pkg_publish_test", true, err)) << err;

    auto pubRes = BuildOrchestrator::publish(m_tempDir.string(), "https://registry.splang.shantopaul.com");
    ASSERT_TRUE(pubRes.success) << pubRes.errorMessage;
    EXPECT_TRUE(fs::exists(pubRes.packageArtifact));
}

TEST_F(BuildOrchestratorTest, SearchRegistry) {
    // Pin the registry URL to an unreachable address: the fallback
    // (built-in offline index) is what this test exercises, and the
    // result must not depend on whether a real registry happens to be
    // listening on localhost:8090 when the suite runs. (Phase 4 slice A)
    const char* oldUrl = std::getenv("SP_REGISTRY_URL");
    std::string savedUrl = oldUrl ? oldUrl : "";
    setenv("SP_REGISTRY_URL", "http://127.0.0.1:9/", 1);

    auto mathResults = BuildOrchestrator::search("math");
    EXPECT_FALSE(mathResults.empty());
    bool foundMath = false;
    for (const auto& r : mathResults) {
        if (r.name == "sp_math") foundMath = true;
    }
    EXPECT_TRUE(foundMath);

    auto jsonResults = BuildOrchestrator::search("json");
    EXPECT_FALSE(jsonResults.empty());
    EXPECT_EQ(jsonResults[0].name, "sp_json");

    auto emptyResults = BuildOrchestrator::search("non_existent_xyz_999");
    EXPECT_TRUE(emptyResults.empty());

    if (savedUrl.empty()) {
        unsetenv("SP_REGISTRY_URL");
    } else {
        setenv("SP_REGISTRY_URL", savedUrl.c_str(), 1);
    }
}

TEST_F(BuildOrchestratorTest, VerifyProjectIntegrityAndTamperDetection) {
    std::string err;
    ASSERT_TRUE(scaffoldProject(m_tempDir.string(), "audit_app", false, err)) << err;

    // 1. Verify project without dependencies
    auto v1 = BuildOrchestrator::verify(m_tempDir.string());
    EXPECT_TRUE(v1.success) << v1.errorMessage;
    EXPECT_EQ(v1.packagesVerified, 0);

    // 2. Add dependencies and create lockfile
    ProjectConfig cfg;
    ASSERT_TRUE(cfg.loadFromFile((m_tempDir / "sp.toml").string()));
    cfg.addDependency(Dependency{"sp_math", "^1.0.0", "", ""});
    ASSERT_TRUE(cfg.saveToFile((m_tempDir / "sp.toml").string()));

    // Verify should fail because sp.lock is missing
    auto v2 = BuildOrchestrator::verify(m_tempDir.string());
    EXPECT_FALSE(v2.success);

    // Create valid lockfile with a REAL content hash (Phase 4 slice A):
    // scaffold a sp_math lib, dry-run publish it (creates the archive
    // and caches it with its SHA-256), and pin the lock to that hash.
    fs::path mathDir = m_tempDir / "sp_math_src";
    ASSERT_TRUE(scaffoldProject(mathDir.string(), "sp_math", true, err)) << err;
    {
        ProjectConfig mathCfg;
        ASSERT_TRUE(mathCfg.loadFromFile((mathDir / "sp.toml").string()));
        mathCfg.getPackage().version = "1.0.0";
        ASSERT_TRUE(mathCfg.saveToFile((mathDir / "sp.toml").string()));
    }
    sp::spm::PublishOptions pubOpts;
    pubOpts.dryRun = true;
    auto pubRes = sp::spm::PackagePublisher::publish(mathDir.string(), pubOpts);
    ASSERT_TRUE(pubRes.success) << pubRes.errorMessage;
    ASSERT_EQ(pubRes.packageVersion, "1.0.0");

    Lockfile lock;
    lock.addPackage(LockedPackage{"sp_math", "1.0.0", "registry", pubRes.sha256Checksum, {}});
    ASSERT_TRUE(lock.saveToFile((m_tempDir / "sp.lock").string()));

    // Verify should now succeed
    auto v3 = BuildOrchestrator::verify(m_tempDir.string());
    EXPECT_TRUE(v3.success) << v3.errorMessage;
    EXPECT_EQ(v3.packagesVerified, 1);

    // 3b. A legacy lockfile (metadata hash instead of content hash) fails.
    Lockfile legacyLock;
    auto semver = *Version::parse("1.0.0");
    legacyLock.addPackage(LockedPackage{"sp_math", "1.0.0", "registry",
                                       DependencyResolver::computeChecksum("sp_math", semver, "registry"), {}});
    ASSERT_TRUE(legacyLock.saveToFile((m_tempDir / "sp.lock").string()));
    auto v3b = BuildOrchestrator::verify(m_tempDir.string());
    EXPECT_FALSE(v3b.success);
    EXPECT_NE(v3b.errorMessage.find("legacy"), std::string::npos);

    // 3. Simulate tampering with checksum
    Lockfile tamperedLock;
    tamperedLock.addPackage(LockedPackage{"sp_math", "1.0.0", "registry", "deadbeef00000000tamperedhash", {}});
    ASSERT_TRUE(tamperedLock.saveToFile((m_tempDir / "sp.lock").string()));

    // Verify MUST detect tampering
    auto v4 = BuildOrchestrator::verify(m_tempDir.string());
    EXPECT_FALSE(v4.success);
    EXPECT_NE(v4.errorMessage.find("tampering detected"), std::string::npos);
}

