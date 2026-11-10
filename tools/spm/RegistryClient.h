// RegistryClient.h — Phase 4 slice A (spm / package ecosystem)
//
// HTTP client for the real SP registry backend (examples/registry-backend/,
// Phase 2 slice D: SP service, SQLite, Bearer auth, SHA-256-verified
// tarballs). Talks to the registry's /api/v1 endpoints over the runtime
// HttpClient (TLS via HttpClientOptions when the URL is https://).
//
// Design notes:
// - IRegistryClient is an interface so unit tests can substitute a stub;
//   HttpRegistryClient is the real implementation.
// - download() never returns bytes whose SHA-256 disagrees with the
//   checksum the registry published for that version: a mismatch is a
//   loud error, never a silent install of wrong bytes.
// - The registry URL resolves as: explicit argument > SP_REGISTRY_URL env
//   > http://127.0.0.1:8090 (local dev default; the registry service
//   binds there by default).
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sp::spm {

// One row of the registry index: metadata for a single name@version.
// `checksum` is the SHA-256 of the package archive bytes as published
// (PackagePublisher computes it over the SP_PACKAGE_ARCHIVE_V1 bytes);
// `tarballStored` is true only after the tarball upload was verified.
struct RegistryPackageInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string license;
    std::string checksum;
    uint64_t archiveSizeBytes{0};
    bool tarballStored{false};
    std::string downloadUrl;
};

// Minimal HTTP client interface for the registry API. All methods return
// false on failure and fill `err` with a human-readable reason (HTTP
// status + body excerpt for server errors, "unreachable" for transport
// failures). No method ever throws.
class IRegistryClient {
public:
    virtual ~IRegistryClient() = default;

    virtual std::string baseUrl() const = 0;

    // GET /api/v1/packages — every published name@version.
    virtual bool listPackages(std::vector<RegistryPackageInfo>& out, std::string& err) = 0;

    // GET /api/v1/packages/{name}/{version} — metadata for one version.
    virtual bool getMetadata(const std::string& name, const std::string& version,
                             RegistryPackageInfo& out, std::string& err) = 0;

    // GET /api/v1/packages/{name}/{version}/download — base64 tarball.
    // Verifies SHA-256(bytes) against the published metadata checksum
    // (and the X-Sha256 response header when present). On mismatch
    // returns false and writes nothing to outBytes.
    virtual bool download(const std::string& name, const std::string& version,
                          std::string& outBytes, std::string& outChecksum, std::string& err) = 0;

    // POST /api/v1/publish — metadata only (JSON). outResponse gets the
    // registry's response body on success.
    virtual bool publishMetadata(const RegistryPackageInfo& info, const std::string& token,
                                 std::string& outResponse, std::string& err) = 0;

    // POST /api/v1/packages/{name}/{version}/tarball — base64 archive bytes.
    // Requires the metadata to exist first (registry answers 404 otherwise).
    virtual bool uploadTarball(const std::string& name, const std::string& version,
                               const std::string& archiveBytes, const std::string& token,
                               std::string& err) = 0;
};

// Real registry client over sp::rt::http::HttpClient.
class HttpRegistryClient : public IRegistryClient {
public:
    // explicitUrl may be empty: then SP_REGISTRY_URL, then the localhost default.
    explicit HttpRegistryClient(std::string explicitUrl = "");

    // Resolve the registry base URL: explicit arg > SP_REGISTRY_URL env
    // > http://127.0.0.1:8090. Trailing '/' is stripped.
    static std::string resolveBaseUrl(const std::string& explicitUrl);
    static std::string defaultBaseUrl();

    std::string baseUrl() const override { return m_baseUrl; }

    bool listPackages(std::vector<RegistryPackageInfo>& out, std::string& err) override;
    bool getMetadata(const std::string& name, const std::string& version,
                     RegistryPackageInfo& out, std::string& err) override;
    bool download(const std::string& name, const std::string& version,
                  std::string& outBytes, std::string& outChecksum, std::string& err) override;
    bool publishMetadata(const RegistryPackageInfo& info, const std::string& token,
                         std::string& outResponse, std::string& err) override;
    bool uploadTarball(const std::string& name, const std::string& version,
                       const std::string& archiveBytes, const std::string& token,
                       std::string& err) override;

    // Pure JSON parsing helpers (no network) — unit-tested directly.
    static bool parsePackageList(const std::string& json, std::vector<RegistryPackageInfo>& out,
                                 std::string& err);
    static bool parsePackageObject(const std::string& json, RegistryPackageInfo& out,
                                   std::string& err);

private:
    struct UrlParts {
        bool useTls{false};
        std::string host;
        int port{80};
        std::string path;
    };
    // Split a full URL into scheme/host/port/path. Returns false on garbage.
    static bool splitUrl(const std::string& url, UrlParts& out);

    // One request with method/body/headers; returns HTTP status (0 = transport failure).
    int request(const std::string& method, const std::string& path,
                const std::string& body, const std::string& contentType,
                const std::string& token, std::string& outBody,
                std::string& outSha256Header, int timeoutMs = 15000);

    static std::string httpError(int status, const std::string& body, const std::string& what);

    std::string m_baseUrl;
};

} // namespace sp::spm
