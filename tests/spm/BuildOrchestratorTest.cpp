#include <gtest/gtest.h>
#include "spm/BuildOrchestrator.h"
#include "spm/ProjectConfig.h"
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

    auto pubRes = BuildOrchestrator::publish(m_tempDir.string(), "https://registry.sp-lang.org");
    ASSERT_TRUE(pubRes.success) << pubRes.errorMessage;
    EXPECT_TRUE(fs::exists(pubRes.packageArtifact));
}

TEST_F(BuildOrchestratorTest, SearchRegistry) {
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
}
