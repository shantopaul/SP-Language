#include "DependencyTree.h"
#include "Cache.h"
#include <sstream>
#include <set>
#include <algorithm>

namespace sp::spm {

namespace {

void renderSubtree(const DependencyTreeNode& node,
                   std::ostringstream& out,
                   const std::string& prefix,
                   bool isLast,
                   bool isRoot,
                   bool useUnicode) {
    if (isRoot) {
        out << node.name << " v" << node.version << "\n";
    } else {
        std::string branch = useUnicode ? (isLast ? "└── " : "├── ") : (isLast ? "\\-- " : "|-- ");
        out << prefix << branch << node.name;
        if (!node.version.empty()) {
            out << " v" << node.version;
        }
        if (!node.versionReq.empty() && node.versionReq != "*") {
            out << " (" << node.versionReq << ")";
        }
        if (!node.source.empty()) {
            out << " [" << node.source << "]";
        }
        if (node.isDuplicate) {
            out << " (*)";
        }
        if (node.hasConflict) {
            out << " [CONFLICT: " << node.conflictReason << "]";
        }
        out << "\n";
    }

    if (node.isDuplicate) {
        return; // don't repeat subtrees
    }

    std::string childPrefix = isRoot ? "" : prefix + (useUnicode ? (isLast ? "    " : "│   ") : (isLast ? "    " : "|   "));
    for (size_t i = 0; i < node.children.size(); ++i) {
        bool childIsLast = (i + 1 == node.children.size());
        renderSubtree(node.children[i], out, childPrefix, childIsLast, false, useUnicode);
    }
}

} // namespace

std::string DependencyTree::formatTree(const DependencyTreeNode& root, bool useUnicode) {
    std::ostringstream out;
    renderSubtree(root, out, "", true, true, useUnicode);
    return out.str();
}

DependencyTreeResult DependencyTree::buildTree(const ProjectConfig& config,
                                              const std::string& projectDir,
                                              std::shared_ptr<IPackageProvider> provider,
                                              int maxDepth) {
    DependencyTreeResult result;
    if (!provider) {
        provider = std::make_shared<DefaultPackageProvider>();
    }

    result.root.name = config.getPackage().name;
    result.root.version = config.getPackage().version;
    result.root.source = projectDir;

    // Track chosen version per package to detect conflicts
    std::unordered_map<std::string, Version> chosenVersions;
    // Track required constraints per package: package -> list of (dependent, requirement)
    std::unordered_map<std::string, std::vector<std::pair<std::string, VersionReq>>> requirements;

    // First resolve the full graph using DependencyResolver
    DependencyResolver resolver(provider);
    auto resolution = resolver.resolve(config, projectDir, true);
    if (!resolution.success) {
        result.success = false;
        result.conflictWarnings.push_back("Failed to resolve dependency graph: " + resolution.errorMessage);
        return result;
    }

    for (const auto& pkg : resolution.resolvedPackages) {
        chosenVersions[pkg.name] = pkg.version;
    }

    // Build the hierarchical tree
    std::set<std::string> globalSeen;

    auto buildNode = [&](auto& self,
                         const std::string& parentName,
                         const Dependency& dep,
                         int depth,
                         std::set<std::string> ancestorPath) -> DependencyTreeNode {
        DependencyTreeNode node;
        node.name = dep.name;
        node.versionReq = dep.versionReq;

        if (ancestorPath.count(dep.name)) {
            node.isDuplicate = true;
            node.conflictReason = "Circular dependency detected";
            node.hasConflict = true;
            return node;
        }

        ancestorPath.insert(dep.name);

        auto it = chosenVersions.find(dep.name);
        if (it != chosenVersions.end()) {
            node.version = it->second.toString();
        }

        if (!dep.path.empty()) {
            node.source = "path: " + dep.path;
        } else if (!dep.gitUrl.empty()) {
            node.source = "git";
        } else {
            node.source = "registry";
        }

        // Check version requirement compatibility
        auto reqOpt = VersionReq::parse(dep.versionReq);
        if (reqOpt && it != chosenVersions.end()) {
            if (!reqOpt->matches(it->second)) {
                node.hasConflict = true;
                node.conflictReason = "Resolved v" + it->second.toString() +
                                      " does not satisfy requested range '" + dep.versionReq + "'";
                result.conflictWarnings.push_back(
                    "Conflict in '" + parentName + "': requires '" + dep.name + " " + dep.versionReq +
                    "' but resolved to v" + it->second.toString()
                );
            }
        }

        // Check if already expanded in global tree
        if (globalSeen.count(dep.name)) {
            node.isDuplicate = true;
            return node;
        }
        globalSeen.insert(dep.name);

        if (depth >= maxDepth) {
            return node;
        }

        // Get sub-dependencies for this package
        const ResolvedPackage* resPkg = resolution.findPackage(dep.name);
        if (resPkg) {
            auto subCfgOpt = provider->getPackageConfig(dep.name, resPkg->version);
            if (subCfgOpt) {
                for (const auto& childDep : subCfgOpt->getDependencies()) {
                    node.children.push_back(self(self, dep.name, childDep, depth + 1, ancestorPath));
                }
            }
        }

        return node;
    };

    std::set<std::string> rootAncestors = {config.getPackage().name};
    for (const auto& dep : config.getDependencies()) {
        result.root.children.push_back(buildNode(buildNode, config.getPackage().name, dep, 1, rootAncestors));
    }
    for (const auto& devDep : config.getDevDependencies()) {
        auto devNode = buildNode(buildNode, config.getPackage().name, devDep, 1, rootAncestors);
        devNode.source += " (dev)";
        result.root.children.push_back(devNode);
    }

    result.formattedTree = formatTree(result.root, true);
    result.success = true;
    return result;
}

} // namespace sp::spm
