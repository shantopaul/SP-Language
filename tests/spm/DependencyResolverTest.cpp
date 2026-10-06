#include <gtest/gtest.h>
#include "spm/SemVer.h"
#include "spm/DependencyResolver.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace sp::spm;

TEST(SemVerTest, ParseAndFormat) {
    auto v1 = Version::parse("1.2.3");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, 1);
    EXPECT_EQ(v1->minor, 2);
    EXPECT_EQ(v1->patch, 3);
    EXPECT_EQ(v1->toString(), "1.2.3");

    auto v2 = Version::parse("0.1.0-alpha.1");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->major, 0);
    EXPECT_EQ(v2->minor, 1);
    EXPECT_EQ(v2->patch, 0);
    EXPECT_EQ(v2->prerelease, "alpha.1");
    EXPECT_EQ(v2->toString(), "0.1.0-alpha.1");

    auto v3 = Version::parse("2.0.0+build2026");
    ASSERT_TRUE(v3.has_value());
    EXPECT_EQ(v3->build, "build2026");

    EXPECT_FALSE(Version::parse("").has_value());
    EXPECT_FALSE(Version::parse("invalid").has_value());
    EXPECT_FALSE(Version::parse("1.2.3.4").has_value());
}

TEST(SemVerTest, ComparisonPrecedence) {
    auto v100 = *Version::parse("1.0.0");
    auto v101 = *Version::parse("1.0.1");
    auto v110 = *Version::parse("1.1.0");
    auto v200 = *Version::parse("2.0.0");

    EXPECT_LT(v100, v101);
    EXPECT_LT(v101, v110);
    EXPECT_LT(v110, v200);

    auto vAlpha = *Version::parse("1.0.0-alpha");
    auto vAlpha1 = *Version::parse("1.0.0-alpha.1");
    auto vBeta = *Version::parse("1.0.0-beta");

    EXPECT_LT(vAlpha, vAlpha1);
    EXPECT_LT(vAlpha1, vBeta);
    EXPECT_LT(vBeta, v100); // prerelease < normal
}

TEST(SemVerTest, VersionRequirementMatching) {
    // Caret ranges
    auto req1 = *VersionReq::parse("^1.2.3");
    EXPECT_TRUE(req1.matches(*Version::parse("1.2.3")));
    EXPECT_TRUE(req1.matches(*Version::parse("1.2.5")));
    EXPECT_TRUE(req1.matches(*Version::parse("1.9.0")));
    EXPECT_FALSE(req1.matches(*Version::parse("1.2.2")));
    EXPECT_FALSE(req1.matches(*Version::parse("2.0.0")));

    auto req0 = *VersionReq::parse("^0.2.3");
    EXPECT_TRUE(req0.matches(*Version::parse("0.2.3")));
    EXPECT_TRUE(req0.matches(*Version::parse("0.2.9")));
    EXPECT_FALSE(req0.matches(*Version::parse("0.3.0")));
    EXPECT_FALSE(req0.matches(*Version::parse("0.2.1")));

    // Tilde ranges
    auto reqTilde = *VersionReq::parse("~1.2.3");
    EXPECT_TRUE(reqTilde.matches(*Version::parse("1.2.3")));
    EXPECT_TRUE(reqTilde.matches(*Version::parse("1.2.8")));
    EXPECT_FALSE(reqTilde.matches(*Version::parse("1.3.0")));

    // Exact
    auto reqExact = *VersionReq::parse("= 1.5.0");
    EXPECT_TRUE(reqExact.matches(*Version::parse("1.5.0")));
    EXPECT_FALSE(reqExact.matches(*Version::parse("1.5.1")));

    // Compound
    auto reqCompound = *VersionReq::parse(">= 1.0.0, < 2.0.0");
    EXPECT_TRUE(reqCompound.matches(*Version::parse("1.0.0")));
    EXPECT_TRUE(reqCompound.matches(*Version::parse("1.9.9")));
    EXPECT_FALSE(reqCompound.matches(*Version::parse("0.9.9")));
    EXPECT_FALSE(reqCompound.matches(*Version::parse("2.0.0")));

    // Wildcard
    auto reqStar = *VersionReq::parse("*");
    EXPECT_TRUE(reqStar.matches(*Version::parse("99.99.99")));
}

TEST(DependencyResolverTest, LinearDependencyResolutionAndTopologicalSort) {
    auto provider = std::make_shared<DefaultPackageProvider>();

    // Mock packages in registry:
    // lib_b: 1.0.0
    ProjectConfig bCfg;
    bCfg.getPackage().name = "lib_b";
    bCfg.getPackage().version = "1.0.0";
    provider->registerMockPackage(bCfg);

    // lib_a: 1.0.0 -> depends on lib_b (^1.0.0)
    ProjectConfig aCfg;
    aCfg.getPackage().name = "lib_a";
    aCfg.getPackage().version = "1.0.0";
    Dependency bDep;
    bDep.name = "lib_b";
    bDep.versionReq = "^1.0.0";
    aCfg.addDependency(bDep);
    provider->registerMockPackage(aCfg);

    // Root app -> depends on lib_a (^1.0.0)
    ProjectConfig rootCfg;
    rootCfg.getPackage().name = "my_app";
    rootCfg.getPackage().version = "0.1.0";
    Dependency aDep;
    aDep.name = "lib_a";
    aDep.versionReq = "^1.0.0";
    rootCfg.addDependency(aDep);

    DependencyResolver resolver(provider);
    auto res = resolver.resolve(rootCfg);

    ASSERT_TRUE(res.success) << res.errorMessage;
    ASSERT_EQ(res.resolvedPackages.size(), 2);

    // Topological order: leaf (lib_b) must come before dependent (lib_a)
    EXPECT_EQ(res.resolvedPackages[0].name, "lib_b");
    EXPECT_EQ(res.resolvedPackages[0].version.toString(), "1.0.0");
    EXPECT_EQ(res.resolvedPackages[1].name, "lib_a");
    EXPECT_EQ(res.resolvedPackages[1].version.toString(), "1.0.0");
}

TEST(DependencyResolverTest, DiamondDependencyResolution) {
    auto provider = std::make_shared<DefaultPackageProvider>();

    // Common library versions: 1.0.0, 1.2.0, 1.3.0
    for (const auto& ver : {"1.0.0", "1.2.0", "1.3.0"}) {
        ProjectConfig cCfg;
        cCfg.getPackage().name = "common_util";
        cCfg.getPackage().version = ver;
        provider->registerMockPackage(cCfg);
    }

    // LibA -> common_util (^1.0.0)
    ProjectConfig aCfg;
    aCfg.getPackage().name = "lib_a";
    aCfg.getPackage().version = "1.0.0";
    Dependency cDep1;
    cDep1.name = "common_util";
    cDep1.versionReq = "^1.0.0";
    aCfg.addDependency(cDep1);
    provider->registerMockPackage(aCfg);

    // LibB -> common_util (^1.2.0)
    ProjectConfig bCfg;
    bCfg.getPackage().name = "lib_b";
    bCfg.getPackage().version = "1.0.0";
    Dependency cDep2;
    cDep2.name = "common_util";
    cDep2.versionReq = "^1.2.0";
    bCfg.addDependency(cDep2);
    provider->registerMockPackage(bCfg);

    // Root app -> LibA and LibB
    ProjectConfig rootCfg;
    rootCfg.getPackage().name = "root_app";
    rootCfg.getPackage().version = "0.1.0";
    Dependency aDep;
    aDep.name = "lib_a";
    aDep.versionReq = "^1.0.0";
    rootCfg.addDependency(aDep);

    Dependency bDep;
    bDep.name = "lib_b";
    bDep.versionReq = "^1.0.0";
    rootCfg.addDependency(bDep);

    DependencyResolver resolver(provider);
    auto res = resolver.resolve(rootCfg);

    ASSERT_TRUE(res.success) << res.errorMessage;
    ASSERT_EQ(res.resolvedPackages.size(), 3);

    // Common util should be resolved to the highest compatible version (1.3.0)
    const auto* commonPkg = res.findPackage("common_util");
    ASSERT_NE(commonPkg, nullptr);
    EXPECT_EQ(commonPkg->version.toString(), "1.3.0");
}

TEST(DependencyResolverTest, CircularDependencyDetection) {
    auto provider = std::make_shared<DefaultPackageProvider>();

    // PkgA -> PkgB
    ProjectConfig aCfg;
    aCfg.getPackage().name = "pkg_a";
    aCfg.getPackage().version = "1.0.0";
    Dependency bDep;
    bDep.name = "pkg_b";
    bDep.versionReq = "^1.0.0";
    aCfg.addDependency(bDep);
    provider->registerMockPackage(aCfg);

    // PkgB -> PkgA (Cycle!)
    ProjectConfig bCfg;
    bCfg.getPackage().name = "pkg_b";
    bCfg.getPackage().version = "1.0.0";
    Dependency aDep;
    aDep.name = "pkg_a";
    aDep.versionReq = "^1.0.0";
    bCfg.addDependency(aDep);
    provider->registerMockPackage(bCfg);

    // Root -> PkgA
    ProjectConfig rootCfg;
    rootCfg.getPackage().name = "root_app";
    rootCfg.getPackage().version = "0.1.0";
    Dependency rootDep;
    rootDep.name = "pkg_a";
    rootDep.versionReq = "^1.0.0";
    rootCfg.addDependency(rootDep);

    DependencyResolver resolver(provider);
    auto res = resolver.resolve(rootCfg);

    EXPECT_FALSE(res.success);
    EXPECT_NE(res.errorMessage.find("Circular dependency detected"), std::string::npos);
}

TEST(DependencyResolverTest, VersionConflictDetection) {
    auto provider = std::make_shared<DefaultPackageProvider>();

    // Common 1.0.0 and 2.0.0
    ProjectConfig c1;
    c1.getPackage().name = "shared_dep";
    c1.getPackage().version = "1.0.0";
    provider->registerMockPackage(c1);

    ProjectConfig c2;
    c2.getPackage().name = "shared_dep";
    c2.getPackage().version = "2.0.0";
    provider->registerMockPackage(c2);

    // LibA requires ^1.0.0
    ProjectConfig aCfg;
    aCfg.getPackage().name = "pkg_a";
    aCfg.getPackage().version = "1.0.0";
    Dependency d1;
    d1.name = "shared_dep";
    d1.versionReq = "^1.0.0";
    aCfg.addDependency(d1);
    provider->registerMockPackage(aCfg);

    // LibB requires ^2.0.0
    ProjectConfig bCfg;
    bCfg.getPackage().name = "pkg_b";
    bCfg.getPackage().version = "1.0.0";
    Dependency d2;
    d2.name = "shared_dep";
    d2.versionReq = "^2.0.0";
    bCfg.addDependency(d2);
    provider->registerMockPackage(bCfg);

    // Root requires both
    ProjectConfig rootCfg;
    rootCfg.getPackage().name = "root_app";
    Dependency r1;
    r1.name = "pkg_a";
    rootCfg.addDependency(r1);
    Dependency r2;
    r2.name = "pkg_b";
    rootCfg.addDependency(r2);

    DependencyResolver resolver(provider);
    auto res = resolver.resolve(rootCfg);

    EXPECT_FALSE(res.success);
    EXPECT_NE(res.errorMessage.find("Version conflict"), std::string::npos);
}

TEST(DependencyResolverTest, LocalPathDependencyResolution) {
    fs::path tempBase = fs::temp_directory_path() / "spm_path_dep_test";
    std::error_code ec;
    fs::remove_all(tempBase, ec);
    fs::create_directories(tempBase / "sub_lib" / "src", ec);
    fs::create_directories(tempBase / "main_app" / "src", ec);

    // Create sub_lib sp.toml
    std::ofstream libToml(tempBase / "sub_lib" / "sp.toml");
    libToml << "[package]\nname = \"sub_lib\"\nversion = \"1.2.0\"\ntype = \"lib\"\n";
    libToml.close();

    // Create main_app sp.toml with path dependency
    ProjectConfig appConfig = ProjectConfig::createDefault("main_app", false);
    Dependency pathDep;
    pathDep.name = "sub_lib";
    pathDep.versionReq = "^1.0.0";
    pathDep.path = "../sub_lib";
    appConfig.addDependency(pathDep);

    DependencyResolver resolver;
    auto res = resolver.resolve(appConfig, (tempBase / "main_app").string());

    ASSERT_TRUE(res.success) << res.errorMessage;
    ASSERT_EQ(res.resolvedPackages.size(), 1);
    EXPECT_EQ(res.resolvedPackages[0].name, "sub_lib");
    EXPECT_EQ(res.resolvedPackages[0].version.toString(), "1.2.0");
    EXPECT_EQ(res.resolvedPackages[0].sourceKind, PackageSourceKind::Path);

    fs::remove_all(tempBase, ec);
}
