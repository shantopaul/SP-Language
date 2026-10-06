#include <gtest/gtest.h>
#include "spm/Lockfile.h"
#include "spm/DependencyResolver.h"
#include "spm/ProjectConfig.h"
#include <filesystem>

namespace fs = std::filesystem;
using namespace sp::spm;

TEST(LockfileTest, RoundTripSerialization) {
    Lockfile lock;
    lock.setVersion(1);

    LockedPackage p1;
    p1.name = "math_util";
    p1.version = "1.2.0";
    p1.source = "path:../math_util";
    p1.checksum = "a1b2c3d4";
    p1.dependencies = {"algo"};
    lock.addPackage(p1);

    LockedPackage p2;
    p2.name = "algo";
    p2.version = "0.9.0";
    p2.source = "registry";
    p2.checksum = "e5f6g7h8";
    p2.dependencies = {};
    lock.addPackage(p2);

    std::string tomlStr = lock.toTomlString();
    EXPECT_NE(tomlStr.find("[[package]]"), std::string::npos);
    EXPECT_NE(tomlStr.find("name = \"math_util\""), std::string::npos);
    EXPECT_NE(tomlStr.find("name = \"algo\""), std::string::npos);

    Lockfile loaded;
    ASSERT_TRUE(loaded.loadFromString(tomlStr));
    EXPECT_EQ(loaded.getVersion(), 1);
    ASSERT_EQ(loaded.getPackages().size(), 2);

    const auto* loadedP1 = loaded.findPackage("math_util");
    ASSERT_NE(loadedP1, nullptr);
    EXPECT_EQ(loadedP1->version, "1.2.0");
    EXPECT_EQ(loadedP1->source, "path:../math_util");
    EXPECT_EQ(loadedP1->checksum, "a1b2c3d4");
    ASSERT_EQ(loadedP1->dependencies.size(), 1);
    EXPECT_EQ(loadedP1->dependencies[0], "algo");

    const auto* loadedP2 = loaded.findPackage("algo");
    ASSERT_NE(loadedP2, nullptr);
    EXPECT_EQ(loadedP2->version, "0.9.0");
    EXPECT_EQ(loadedP2->source, "registry");
    EXPECT_TRUE(loadedP2->dependencies.empty());
}

TEST(LockfileTest, FromResolution) {
    ResolutionResult res;
    res.success = true;

    ResolvedPackage r1;
    r1.name = "core";
    r1.version = *Version::parse("2.1.0");
    r1.sourceKind = PackageSourceKind::Registry;
    r1.sourceLocation = "registry";
    r1.checksum = "deadbeef";
    r1.dependencies = {};
    res.resolvedPackages.push_back(r1);

    Lockfile lock = Lockfile::fromResolution(res);
    ASSERT_EQ(lock.getPackages().size(), 1);
    EXPECT_EQ(lock.getPackages()[0].name, "core");
    EXPECT_EQ(lock.getPackages()[0].version, "2.1.0");
    EXPECT_EQ(lock.getPackages()[0].source, "registry");
    EXPECT_EQ(lock.getPackages()[0].checksum, "deadbeef");
}

TEST(LockfileTest, IsUpToDateChecks) {
    Lockfile lock;
    LockedPackage p;
    p.name = "logger";
    p.version = "1.4.2";
    p.source = "registry";
    p.checksum = "123456";
    lock.addPackage(p);

    // Matching project config
    ProjectConfig config = ProjectConfig::createDefault("my_project", false);
    Dependency dep;
    dep.name = "logger";
    dep.versionReq = "^1.0.0";
    config.addDependency(dep);

    EXPECT_TRUE(lock.isUpToDate(config));

    // Incompatible requirement (^2.0.0)
    config.removeDependency("logger");
    dep.versionReq = "^2.0.0";
    config.addDependency(dep);
    EXPECT_FALSE(lock.isUpToDate(config));

    // New missing dependency
    Dependency dep2;
    dep2.name = "network";
    dep2.versionReq = "*";
    config.addDependency(dep2);
    EXPECT_FALSE(lock.isUpToDate(config));
}

TEST(LockfileTest, SaveAndLoadFromFile) {
    fs::path tempFile = fs::temp_directory_path() / "test_sp.lock";

    Lockfile lock;
    LockedPackage p;
    p.name = "json";
    p.version = "0.5.1";
    p.source = "registry";
    p.checksum = "feedface";
    lock.addPackage(p);

    ASSERT_TRUE(lock.saveToFile(tempFile.string()));

    Lockfile loaded;
    ASSERT_TRUE(loaded.loadFromFile(tempFile.string()));
    ASSERT_EQ(loaded.getPackages().size(), 1);
    EXPECT_EQ(loaded.getPackages()[0].name, "json");
    EXPECT_EQ(loaded.getPackages()[0].version, "0.5.1");

    std::error_code ec;
    fs::remove(tempFile, ec);
}
