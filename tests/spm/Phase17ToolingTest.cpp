#include <gtest/gtest.h>
#include "spm/ProjectConfig.h"
#include "spm/DependencyResolver.h"
#include "spm/BuildOrchestrator.h"
#include "spm/Cache.h"
#include "spm/Publish.h"
#include "spm/DependencyTree.h"
#include "sp/runtime/Crypto.h"

#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;
using namespace sp::spm;

class Phase17ToolingTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tempDir = fs::temp_directory_path() / ("spm_p17_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
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

// 1. Offline Cache Test
TEST_F(Phase17ToolingTest, OfflineCacheStoreVerifyAndClean) {
    fs::path pkgSrc = m_tempDir / "sample_pkg";
    fs::create_directories(pkgSrc / "src");

    std::ofstream toml(pkgSrc / "sp.toml");
    toml << "[package]\nname = \"sample_pkg\"\nversion = \"1.0.0\"\n";
    toml.close();

    std::ofstream code(pkgSrc / "src" / "main.sp");
    code << "fn main() { }\n";
    code.close();

    std::string expectedChecksum = sp::rt::crypto::sha256("sample_pkg@1.0.0");

    // Store in cache
    std::string err;
    EXPECT_TRUE(PackageCache::storePackage("sample_pkg", "1.0.0", pkgSrc, expectedChecksum, err)) << err;

    // Verify cache has it
    EXPECT_TRUE(PackageCache::isCached("sample_pkg", "1.0.0"));
    EXPECT_TRUE(PackageCache::verifyChecksum("sample_pkg", "1.0.0", expectedChecksum));
    EXPECT_EQ(PackageCache::getCachedChecksum("sample_pkg", "1.0.0"), expectedChecksum);

    // List cached packages
    auto list = PackageCache::listCachedPackages();
    bool foundInList = false;
    for (const auto& [n, v] : list) {
        if (n == "sample_pkg" && v == "1.0.0") foundInList = true;
    }
    EXPECT_TRUE(foundInList);
    EXPECT_GT(PackageCache::getCacheSizeBytes(), 0);

    // Clean cache
    EXPECT_TRUE(PackageCache::clean(err)) << err;
    EXPECT_FALSE(PackageCache::isCached("sample_pkg", "1.0.0"));
}

// 2. Dependency Tree and Conflict Diagnostics Test
TEST_F(Phase17ToolingTest, DependencyTreeAndConflictDetection) {
    ProjectConfig rootConfig;
    rootConfig.getPackage().name = "app_tree_test";
    rootConfig.getPackage().version = "2.1.0";
    rootConfig.addDependency(Dependency{"sp_math", "^1.0.0", "", ""});
    rootConfig.addDependency(Dependency{"sp_http", "^0.4.0", "", ""});

    auto treeRes = DependencyTree::buildTree(rootConfig, m_tempDir.string());
    EXPECT_TRUE(treeRes.success);
    EXPECT_EQ(treeRes.root.name, "app_tree_test");
    EXPECT_EQ(treeRes.root.version, "2.1.0");
    EXPECT_FALSE(treeRes.formattedTree.empty());

    // Verify formatted tree contains tree characters and package names
    EXPECT_NE(treeRes.formattedTree.find("app_tree_test"), std::string::npos);
    EXPECT_NE(treeRes.formattedTree.find("sp_math"), std::string::npos);
    EXPECT_NE(treeRes.formattedTree.find("sp_http"), std::string::npos);

    // Test conflict detection
    ProjectConfig conflictConfig;
    conflictConfig.getPackage().name = "conflict_app";
    conflictConfig.getPackage().version = "1.0.0";
    conflictConfig.addDependency(Dependency{"sp_math", "^99.0.0", "", ""}); // Impossible version

    auto conflictRes = DependencyTree::buildTree(conflictConfig, m_tempDir.string());
    EXPECT_FALSE(conflictRes.success);
    EXPECT_FALSE(conflictRes.conflictWarnings.empty());
}

// 3. Package Publisher Test
TEST_F(Phase17ToolingTest, PackagePublisherTarballAndChecksum) {
    std::string err;
    ASSERT_TRUE(scaffoldProject(m_tempDir.string(), "publish_sample", false, err)) << err;

    PublishOptions opts;
    opts.dryRun = true;
    opts.registryUrl = "https://registry.splang.shantopaul.com";

    auto pubRes = PackagePublisher::publish(m_tempDir.string(), opts);
    EXPECT_TRUE(pubRes.success) << pubRes.errorMessage;
    EXPECT_EQ(pubRes.packageName, "publish_sample");
    EXPECT_FALSE(pubRes.sha256Checksum.empty());
    EXPECT_EQ(pubRes.sha256Checksum.size(), 64); // Valid 64-char SHA-256
    EXPECT_TRUE(fs::exists(pubRes.packageArtifact));
    EXPECT_GT(pubRes.archiveSizeBytes, 0);

    // Compute checksum independently and assert match
    std::string verifiedSha = PackagePublisher::computeFileSha256(pubRes.packageArtifact);
    EXPECT_EQ(verifiedSha, pubRes.sha256Checksum);
}

// 4. Workspace Monorepo Test
TEST_F(Phase17ToolingTest, WorkspaceMonorepoConfigAndBuild) {
    // 1. Create workspace root
    std::vector<std::string> members = {"packages/core", "packages/cli"};
    ProjectConfig wsConfig = ProjectConfig::createWorkspace(members);
    EXPECT_TRUE(wsConfig.isWorkspace());
    EXPECT_EQ(wsConfig.getWorkspaceMembers().size(), 2);

    std::string tomlStr = wsConfig.toTomlString();
    EXPECT_NE(tomlStr.find("[workspace]"), std::string::npos);
    EXPECT_NE(tomlStr.find("packages/core"), std::string::npos);

    // Test round-trip loadFromString
    ProjectConfig parsedWs;
    EXPECT_TRUE(parsedWs.loadFromString(tomlStr));
    EXPECT_TRUE(parsedWs.isWorkspace());
    EXPECT_EQ(parsedWs.getWorkspaceMembers().size(), 2);
    EXPECT_EQ(parsedWs.getWorkspaceMembers()[0], "packages/core");
    EXPECT_EQ(parsedWs.getWorkspaceMembers()[1], "packages/cli");

    // 2. Scaffold workspace structure on disk
    fs::path coreDir = m_tempDir / "packages" / "core";
    fs::path cliDir = m_tempDir / "packages" / "cli";
    std::string scaffoldErr;
    ASSERT_TRUE(scaffoldProject(coreDir.string(), "core", true, scaffoldErr)) << scaffoldErr;
    ASSERT_TRUE(scaffoldProject(cliDir.string(), "cli", false, scaffoldErr)) << scaffoldErr;

    // Save root sp.toml in m_tempDir
    ASSERT_TRUE(wsConfig.saveToFile((m_tempDir / "sp.toml").string()));

    // 3. Build workspace
    BuildOptions bOpts;
    auto wsBuildRes = BuildOrchestrator::buildWorkspace(m_tempDir.string(), bOpts);
    EXPECT_TRUE(wsBuildRes.success) << wsBuildRes.errorMessage;
    EXPECT_EQ(wsBuildRes.memberBinaries.size(), 2);
}
