#pragma once

#include "ProjectConfig.h"
#include <string>
#include <vector>

namespace sp::spm {

struct PublishOptions {
    std::string registryUrl{"https://registry.splang.shantopaul.com"};
    std::string token;
    bool dryRun{false};
    bool skipBuildValidation{false};
};

struct PublishResult {
    bool success{false};
    std::string packageName;
    std::string packageVersion;
    std::string packageArtifact;
    std::string sha256Checksum;
    size_t archiveSizeBytes{0};
    std::string registryResponse;
    std::string errorMessage;
};

class PackagePublisher {
public:
    static PublishResult publish(const std::string& projectDir, const PublishOptions& options);

    // Compute SHA-256 checksum of any file on disk
    static std::string computeFileSha256(const std::string& filePath);

    // Retrieve active API token from options, environment (SP_REGISTRY_TOKEN), or credentials
    static std::string resolveApiToken(const PublishOptions& options);
};

} // namespace sp::spm
