// RegistryClient.cpp — Phase 4 slice A (spm / package ecosystem)
#include "RegistryClient.h"
#include "sp/Version.h"
#include "sp/runtime/Crypto.h"
#include "sp/runtime/Http.h"
#include "sp/runtime/Json.h"

#include <cstdlib>
#include <sstream>

namespace sp::spm {

namespace {

std::string trimSlash(const std::string& s) {
    std::string out = s;
    while (!out.empty() && out.back() == '/') out.pop_back();
    return out;
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string getStringField(const sp::rt::json::JsonValuePtr& obj, const char* key) {
    if (!obj || !obj->isObject()) return "";
    auto v = obj->get(key);
    if (!v || !v->isString()) return "";
    return v->asString();
}

} // namespace

std::string HttpRegistryClient::defaultBaseUrl() {
    return "http://127.0.0.1:8090";
}

std::string HttpRegistryClient::resolveBaseUrl(const std::string& explicitUrl) {
    if (!explicitUrl.empty()) return trimSlash(explicitUrl);
    const char* env = std::getenv("SP_REGISTRY_URL");
    if (env && *env) return trimSlash(env);
    return defaultBaseUrl();
}

HttpRegistryClient::HttpRegistryClient(std::string explicitUrl)
    : m_baseUrl(resolveBaseUrl(explicitUrl)) {}

bool HttpRegistryClient::splitUrl(const std::string& url, UrlParts& out) {
    std::string rest;
    if (url.rfind("https://", 0) == 0) {
        out.useTls = true;
        rest = url.substr(8);
    } else if (url.rfind("http://", 0) == 0) {
        out.useTls = false;
        rest = url.substr(7);
    } else {
        return false;
    }
    size_t slash = rest.find('/');
    std::string hostPort = (slash == std::string::npos) ? rest : rest.substr(0, slash);
    out.path = (slash == std::string::npos) ? "/" : rest.substr(slash);
    out.port = out.useTls ? 443 : 80;
    size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        out.host = hostPort.substr(0, colon);
        try {
            out.port = std::stoi(hostPort.substr(colon + 1));
        } catch (const std::exception&) {
            return false;
        }
    } else {
        out.host = hostPort;
    }
    if (out.host.empty() || out.port <= 0 || out.port > 65535) return false;
    return true;
}

std::string HttpRegistryClient::httpError(int status, const std::string& body, const std::string& what) {
    std::ostringstream oss;
    if (status == 0) {
        oss << what << ": registry unreachable (connection failed)";
        return oss.str();
    }
    // The runtime HTTP client reports a refused connection as HTTP 500
    // with a "Failed to connect" body — phrase that as unreachable
    // instead of a server error.
    if (body.find("Failed to connect") != std::string::npos) {
        oss << what << ": registry unreachable (" << body.substr(0, 120) << ")";
        return oss.str();
    }
    oss << what << ": registry answered HTTP " << status;
    if (!body.empty()) {
        oss << ": " << body.substr(0, 200);
    }
    return oss.str();
}

int HttpRegistryClient::request(const std::string& method, const std::string& path,
                                const std::string& body, const std::string& contentType,
                                const std::string& token, std::string& outBody,
                                std::string& outSha256Header, int timeoutMs) {
    UrlParts parts;
    if (!splitUrl(m_baseUrl + path, parts)) return 0;

    sp::rt::http::HttpRequest req;
    req.method = method;
    req.path = parts.path;
    req.body = body;
    if (!contentType.empty()) req.setHeader("Content-Type", contentType);
    if (!token.empty()) req.setHeader("Authorization", "Bearer " + token);
    req.setHeader("User-Agent", "spm/" + std::string(sp::SP_VERSION_STRING));

    sp::rt::http::HttpClientOptions opts;
    opts.timeoutMs = timeoutMs;
    opts.useTls = parts.useTls;
    auto resp = sp::rt::http::HttpClient::send(req, parts.host, parts.port, opts);
    outBody = resp.body;
    outSha256Header = resp.getHeader("X-Sha256");
    return resp.statusCode;
}

bool HttpRegistryClient::parsePackageObject(const std::string& json, RegistryPackageInfo& out,
                                            std::string& err) {
    std::string parseErr;
    auto root = sp::rt::json::parse(json, &parseErr);
    if (!root || !root->isObject()) {
        err = "invalid registry JSON: " + parseErr;
        return false;
    }
    out.name = getStringField(root, "name");
    out.version = getStringField(root, "version");
    out.description = getStringField(root, "description");
    out.license = getStringField(root, "license");
    out.checksum = getStringField(root, "checksum");
    out.downloadUrl = getStringField(root, "downloadUrl");
    if (auto v = root->get("archiveSizeBytes"); v && v->isNumber()) {
        out.archiveSizeBytes = static_cast<uint64_t>(v->asInt());
    }
    if (auto v = root->get("tarballStored"); v && v->isBool()) {
        out.tarballStored = v->asBool();
    }
    if (out.name.empty() || out.version.empty()) {
        err = "registry JSON is missing name/version: " + json.substr(0, 200);
        return false;
    }
    return true;
}

bool HttpRegistryClient::parsePackageList(const std::string& json,
                                          std::vector<RegistryPackageInfo>& out,
                                          std::string& err) {
    std::string parseErr;
    auto root = sp::rt::json::parse(json, &parseErr);
    if (!root || !root->isObject()) {
        err = "invalid registry JSON: " + parseErr;
        return false;
    }
    auto arr = root->get("packages");
    if (!arr || !arr->isArray()) {
        err = "registry list JSON has no 'packages' array";
        return false;
    }
    out.clear();
    for (const auto& item : arr->getArrayItems()) {
        RegistryPackageInfo info;
        if (!item || !item->isObject()) continue;
        info.name = getStringField(item, "name");
        info.version = getStringField(item, "version");
        info.description = getStringField(item, "description");
        info.license = getStringField(item, "license");
        info.checksum = getStringField(item, "checksum");
        info.downloadUrl = getStringField(item, "downloadUrl");
        if (auto v = item->get("archiveSizeBytes"); v && v->isNumber()) {
            info.archiveSizeBytes = static_cast<uint64_t>(v->asInt());
        }
        if (auto v = item->get("tarballStored"); v && v->isBool()) {
            info.tarballStored = v->asBool();
        }
        if (!info.name.empty() && !info.version.empty()) out.push_back(std::move(info));
    }
    return true;
}

bool HttpRegistryClient::listPackages(std::vector<RegistryPackageInfo>& out, std::string& err) {
    std::string body, sha;
    int status = request("GET", "/api/v1/packages", "", "", "", body, sha, 5000);
    if (status < 200 || status >= 300) {
        err = httpError(status, body, "GET /api/v1/packages");
        return false;
    }
    return parsePackageList(body, out, err);
}

bool HttpRegistryClient::getMetadata(const std::string& name, const std::string& version,
                                     RegistryPackageInfo& out, std::string& err) {
    std::string body, sha;
    int status = request("GET", "/api/v1/packages/" + name + "/" + version, "", "", "", body, sha, 5000);
    if (status == 404) {
        err = "package '" + name + "@" + version + "' not found in registry at " + m_baseUrl;
        return false;
    }
    if (status < 200 || status >= 300) {
        err = httpError(status, body, "GET /api/v1/packages/" + name + "/" + version);
        return false;
    }
    return parsePackageObject(body, out, err);
}

bool HttpRegistryClient::download(const std::string& name, const std::string& version,
                                  std::string& outBytes, std::string& outChecksum,
                                  std::string& err) {
    // 1. Published checksum first: the bytes must match it, or they are refused.
    RegistryPackageInfo meta;
    if (!getMetadata(name, version, meta, err)) return false;
    if (meta.checksum.empty()) {
        err = "registry metadata for '" + name + "@" + version + "' has no checksum; refusing download";
        return false;
    }
    if (!meta.tarballStored) {
        err = "package '" + name + "@" + version + "' has no verified tarball in the registry "
              "(published metadata only; the publisher never uploaded the tarball)";
        return false;
    }

    // 2. Download the base64 tarball.
    std::string body, headerSha;
    int status = request("GET", "/api/v1/packages/" + name + "/" + version + "/download",
                         "", "", "", body, headerSha, 30000);
    if (status == 404) {
        err = "no tarball stored for '" + name + "@" + version + "' in the registry";
        return false;
    }
    if (status < 200 || status >= 300) {
        err = httpError(status, body, "GET /api/v1/packages/" + name + "/" + version + "/download");
        return false;
    }

    std::string bytes = sp::rt::crypto::base64Decode(body);
    if (bytes.empty() && !body.empty()) {
        err = "registry download for '" + name + "@" + version + "' is not valid base64";
        return false;
    }
    std::string computed = sp::rt::crypto::sha256(bytes);
    if (computed != meta.checksum) {
        err = "downloaded tarball for '" + name + "@" + version + "' fails SHA-256 verification "
              "(expected " + meta.checksum + ", got " + computed + "); refusing to install";
        return false;
    }
    if (!headerSha.empty() && headerSha != computed) {
        err = "downloaded tarball for '" + name + "@" + version + "' disagrees with the "
              "X-Sha256 response header; refusing to install";
        return false;
    }
    outBytes = std::move(bytes);
    outChecksum = computed;
    return true;
}

bool HttpRegistryClient::publishMetadata(const RegistryPackageInfo& info, const std::string& token,
                                         std::string& outResponse, std::string& err) {
    std::ostringstream payload;
    payload << "{\n"
            << "  \"name\": \"" << jsonEscape(info.name) << "\",\n"
            << "  \"version\": \"" << jsonEscape(info.version) << "\",\n"
            << "  \"description\": \"" << jsonEscape(info.description) << "\",\n"
            << "  \"license\": \"" << jsonEscape(info.license.empty() ? "MIT" : info.license) << "\",\n"
            << "  \"checksum\": \"" << jsonEscape(info.checksum) << "\",\n"
            << "  \"archiveSizeBytes\": " << info.archiveSizeBytes << "\n"
            << "}";
    std::string body, sha;
    int status = request("POST", "/api/v1/publish", payload.str(), "application/json", token, body, sha);
    if (status < 200 || status >= 300) {
        err = httpError(status, body, "POST /api/v1/publish");
        return false;
    }
    outResponse = body;
    return true;
}

bool HttpRegistryClient::uploadTarball(const std::string& name, const std::string& version,
                                       const std::string& archiveBytes, const std::string& token,
                                       std::string& err) {
    std::string b64 = sp::rt::crypto::base64Encode(archiveBytes);
    std::string body, sha;
    int status = request("POST", "/api/v1/packages/" + name + "/" + version + "/tarball",
                         b64, "text/plain", token, body, sha);
    if (status == 404) {
        err = "tarball upload failed: package '" + name + "@" + version +
              "' has no metadata in the registry (publish metadata first)";
        return false;
    }
    if (status < 200 || status >= 300) {
        err = httpError(status, body, "POST /api/v1/packages/" + name + "/" + version + "/tarball");
        return false;
    }
    return true;
}

} // namespace sp::spm
