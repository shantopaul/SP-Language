#pragma once

#include "ProjectConfig.h"
#include "Lockfile.h"
#include "DependencyResolver.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace sp::spm {

struct DependencyTreeNode {
    std::string name;
    std::string version;
    std::string versionReq;
    std::string source;
    bool isDuplicate{false};
    bool hasConflict{false};
    std::string conflictReason;
    std::vector<DependencyTreeNode> children;
};

struct DependencyTreeResult {
    bool success{false};
    DependencyTreeNode root;
    std::vector<std::string> conflictWarnings;
    std::string formattedTree;
};

class DependencyTree {
public:
    static DependencyTreeResult buildTree(const ProjectConfig& config,
                                          const std::string& projectDir = ".",
                                          std::shared_ptr<IPackageProvider> provider = nullptr,
                                          int maxDepth = 100);

    static std::string formatTree(const DependencyTreeNode& root, bool useUnicode = true);
};

} // namespace sp::spm
