#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace sp::rt::crypto {

enum class PasswordAlgorithm {
    Argon2id,
    Bcrypt
};

struct PasswordOptions {
    PasswordAlgorithm algorithm = PasswordAlgorithm::Argon2id;
    // Argon2id parameters (RFC 9106)
    uint32_t memoryKiB = 65536; // 64 MB default (or lower for testing e.g. 1024 / 4096)
    uint32_t iterations = 3;
    uint32_t parallelism = 1;
    uint32_t hashLength = 32;

    // Bcrypt parameters
    uint32_t bcryptCost = 10;   // Cost factor (4 to 31, default 10)
};

class Password {
public:
    /// Hash a plain-text password using the configured algorithm and options.
    /// Returns standard PHC string for Argon2id ($argon2id$v=19$...) or Modular Crypt Format for Bcrypt ($2a$...).
    static std::string hash(const std::string& plainText, const PasswordOptions& options = {});

    /// Verify a plain-text password against a stored PHC / MCF hash string using constant-time comparison.
    /// Automatically detects algorithm from hash prefix ($argon2id$ vs $2a$ / $2b$).
    static bool verify(const std::string& plainText, const std::string& hash);

    /// Checks if a hash was computed with older/weaker parameters than desired.
    static bool needsRehash(const std::string& hash, const PasswordOptions& targetOptions = {});

    // Low-level Argon2id (RFC 9106)
    static std::vector<uint8_t> argon2idRaw(
        const std::string& password,
        const std::vector<uint8_t>& salt,
        uint32_t iterations,
        uint32_t memoryKiB,
        uint32_t parallelism,
        uint32_t tagLength = 32
    );

    // Low-level Bcrypt
    static std::string bcryptHash(
        const std::string& password,
        const std::vector<uint8_t>& salt16,
        uint32_t cost = 10
    );
};

} // namespace sp::rt::crypto
