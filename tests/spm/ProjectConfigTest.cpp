#include <gtest/gtest.h>
#include "spm/ProjectConfig.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace sp::spm::test {

TEST(ProjectConfigTest, CreateDefaultBinaryPackage) {
    auto config = ProjectConfig::createDefault("my_cool_app", false);
    EXPECT_EQ(config.getPackage().name, "my_cool_app");
    EXPECT_EQ(config.getPackage().version, "0.1.0");
    EXPECT_EQ(config.getPackage().edition, "2026");
    EXPECT_EQ(config.getPackage().packageType, "bin");
    EXPECT_EQ(config.getPackage().entry, "src/main.sp");
    EXPECT_TRUE(config.getDependencies().empty());
}

TEST(ProjectConfigTest, CreateDefaultLibraryPackage) {
    auto config = ProjectConfig::createDefault("my_math_lib", true);
    EXPECT_EQ(config.getPackage().name, "my_math_lib");
    EXPECT_EQ(config.getPackage().packageType, "lib");
    EXPECT_EQ(config.getPackage().entry, "src/lib.sp");
}

TEST(ProjectConfigTest, ParseTomlString) {
    const std::string toml = R"toml(
        # SP project configuration
        [package]
        name = "demo_service"
        version = "1.2.3"
        description = "A demo web service"
        author = "Shanto Paul"
        edition = "2026"
        entry = "src/main.sp"
        type = "bin"

        [dependencies]
        std_http = "^0.4.0"
        local_lib = { path = "../local_lib", version = "1.0.0" }

        [dev-dependencies]
        sp_test_utils = "0.1.0"
    )toml";

    ProjectConfig config;
    ASSERT_TRUE(config.loadFromString(toml)) << config.getLastError();

    EXPECT_EQ(config.getPackage().name, "demo_service");
    EXPECT_EQ(config.getPackage().version, "1.2.3");
    EXPECT_EQ(config.getPackage().description, "A demo web service");
    EXPECT_EQ(config.getPackage().author, "Shanto Paul");
    EXPECT_EQ(config.getPackage().packageType, "bin");

    ASSERT_EQ(config.getDependencies().size(), 2);
    auto* httpDep = config.findDependency("std_http");
    ASSERT_NE(httpDep, nullptr);
    EXPECT_EQ(httpDep->versionReq, "^0.4.0");
    EXPECT_TRUE(httpDep->path.empty());

    auto* localDep = config.findDependency("local_lib");
    ASSERT_NE(localDep, nullptr);
    EXPECT_EQ(localDep->versionReq, "1.0.0");
    EXPECT_EQ(localDep->path, "../local_lib");

    ASSERT_EQ(config.getDevDependencies().size(), 1);
    auto* devDep = config.findDependency("sp_test_utils", true);
    ASSERT_NE(devDep, nullptr);
    EXPECT_EQ(devDep->versionReq, "0.1.0");
}

TEST(ProjectConfigTest, AddAndRemoveDependencies) {
    auto config = ProjectConfig::createDefault("cli_tool");

    Dependency d1;
    d1.name = "serde";
    d1.versionReq = "^2.0.0";
    EXPECT_TRUE(config.addDependency(d1));

    Dependency d2;
    d2.name = "log";
    d2.versionReq = "0.4.1";
    EXPECT_TRUE(config.addDependency(d2));

    EXPECT_EQ(config.getDependencies().size(), 2);
    EXPECT_TRUE(config.hasDependency("serde"));
    EXPECT_TRUE(config.hasDependency("log"));

    EXPECT_TRUE(config.removeDependency("serde"));
    EXPECT_EQ(config.getDependencies().size(), 1);
    EXPECT_FALSE(config.hasDependency("serde"));
    EXPECT_TRUE(config.hasDependency("log"));
}

TEST(ProjectConfigTest, RoundTripTomlSerialization) {
    ProjectConfig original = ProjectConfig::createDefault("roundtrip_test");
    original.getPackage().version = "2.5.0";
    original.getPackage().description = "Roundtrip serialization test";

    Dependency d1;
    d1.name = "std_json";
    d1.versionReq = "^1.1.0";
    original.addDependency(d1);

    Dependency d2;
    d2.name = "internal_util";
    d2.path = "../utils";
    original.addDependency(d2);

    std::string tomlStr = original.toTomlString();

    ProjectConfig loaded;
    ASSERT_TRUE(loaded.loadFromString(tomlStr));

    EXPECT_EQ(loaded.getPackage().name, "roundtrip_test");
    EXPECT_EQ(loaded.getPackage().version, "2.5.0");
    EXPECT_EQ(loaded.getPackage().description, "Roundtrip serialization test");
    ASSERT_EQ(loaded.getDependencies().size(), 2);

    auto* loadedD1 = loaded.findDependency("std_json");
    ASSERT_NE(loadedD1, nullptr);
    EXPECT_EQ(loadedD1->versionReq, "^1.1.0");

    auto* loadedD2 = loaded.findDependency("internal_util");
    ASSERT_NE(loadedD2, nullptr);
    EXPECT_EQ(loadedD2->path, "../utils");
}

TEST(ProjectConfigTest, NativeLibsParseAndRoundTrip) {
    // Phase 39 slice B: [native] libs + lib-paths, single-line and
    // multi-line arrays, and a toTomlString round-trip.
    const std::string toml = R"toml(
        [package]
        name = "ffi_app"
        version = "0.1.0"
        edition = "2026"
        entry = "src/main.sp"
        type = "bin"

        [native]
        libs = ["mini", "z"]
        lib-paths = [
            "./vendor/lib",
            "/usr/local/lib",
        ]
    )toml";

    ProjectConfig config;
    ASSERT_TRUE(config.loadFromString(toml)) << config.getLastError();

    ASSERT_EQ(config.getNativeLibs().size(), 2);
    EXPECT_EQ(config.getNativeLibs()[0], "mini");
    EXPECT_EQ(config.getNativeLibs()[1], "z");
    ASSERT_EQ(config.getNativeLibPaths().size(), 2);
    EXPECT_EQ(config.getNativeLibPaths()[0], "./vendor/lib");
    EXPECT_EQ(config.getNativeLibPaths()[1], "/usr/local/lib");

    std::string roundTripped = config.toTomlString();
    ProjectConfig loaded;
    ASSERT_TRUE(loaded.loadFromString(roundTripped)) << loaded.getLastError();
    EXPECT_EQ(loaded.getNativeLibs(), config.getNativeLibs());
    EXPECT_EQ(loaded.getNativeLibPaths(), config.getNativeLibPaths());
}

TEST(ProjectConfigTest, NativeLibsAbsentByDefault) {
    auto config = ProjectConfig::createDefault("plain_app");
    EXPECT_TRUE(config.getNativeLibs().empty());
    EXPECT_TRUE(config.getNativeLibPaths().empty());
    // And the section is omitted from the serialization.
    EXPECT_EQ(config.toTomlString().find("[native]"), std::string::npos);
}

TEST(ProjectConfigTest, ScaffoldBinaryAndLibraryProjects) {
    std::string tempBinDir = "temp_test_scaffold_bin";
    std::string tempLibDir = "temp_test_scaffold_lib";

    std::error_code ec;
    fs::remove_all(tempBinDir, ec);
    fs::remove_all(tempLibDir, ec);

    // 1. Binary project scaffold
    std::string err;
    bool okBin = scaffoldProject(tempBinDir, "sample_bin", false, err);
    ASSERT_TRUE(okBin) << err;
    EXPECT_TRUE(fs::exists(fs::path(tempBinDir) / "sp.toml"));
    EXPECT_TRUE(fs::exists(fs::path(tempBinDir) / "src" / "main.sp"));
    EXPECT_TRUE(fs::exists(fs::path(tempBinDir) / ".gitignore"));

    ProjectConfig binConfig;
    ASSERT_TRUE(binConfig.loadFromFile((fs::path(tempBinDir) / "sp.toml").string()));
    EXPECT_EQ(binConfig.getPackage().name, "sample_bin");
    EXPECT_EQ(binConfig.getPackage().packageType, "bin");

    // 2. Library project scaffold
    bool okLib = scaffoldProject(tempLibDir, "sample_lib", true, err);
    ASSERT_TRUE(okLib) << err;
    EXPECT_TRUE(fs::exists(fs::path(tempLibDir) / "sp.toml"));
    EXPECT_TRUE(fs::exists(fs::path(tempLibDir) / "src" / "lib.sp"));

    ProjectConfig libConfig;
    ASSERT_TRUE(libConfig.loadFromFile((fs::path(tempLibDir) / "sp.toml").string()));
    EXPECT_EQ(libConfig.getPackage().name, "sample_lib");
    EXPECT_EQ(libConfig.getPackage().packageType, "lib");

    // Clean up
    fs::remove_all(tempBinDir, ec);
    fs::remove_all(tempLibDir, ec);
}

} // namespace sp::spm::test
