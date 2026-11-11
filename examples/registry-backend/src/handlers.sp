module handlers;

import std.web;
import std.json;
import std.string;
import std.fs;
import std.crypto;
import config;
import auth;
import store;
import json_util;

// ============================================================================
// HTTP handlers for the SP registry service.
//
// Every response is built from data actually read from SQLite (or from the
// request itself). Nothing is hard-coded: verify recomputes the tarball
// hash at request time, publish stores the metadata spm sent, and download
// returns the tarball bytes that were actually uploaded — or an honest 404
// when no tarball was uploaded yet (spm publish sends metadata only).
// ============================================================================

public class RegistryHandlers {
    public static fn health(Context ctx) -> HttpResponse {
        let count = RegistryStore.countPackages(RegistryConfig.dbPath());
        let json = "{\"status\":\"ok\",\"service\":\"sp-registry\",\"version\":\"4.0.0\",\"backend\":\"sp\",\"database\":\"sqlite\",\"packages\":" + count + "}";
        return ctx.json(json);
    }

    public static fn publish(Context ctx) -> HttpResponse {
        if (!RegistryAuth.validateBearer(ctx, RegistryConfig.token())) {
            return RegistryAuth.unauthorizedResponse(ctx);
        }
        let body = ctx.body();
        if (body == "") {
            return ctx.jsonStatus("{\"error\":\"Empty request body\",\"code\":\"BAD_REQUEST\"}", 400);
        }
        let j = Json.parse(body);
        if (!j.isValid()) {
            return ctx.jsonStatus("{\"error\":\"Invalid JSON publication payload\",\"code\":\"BAD_REQUEST\"}", 400);
        }
        if (!j.has("name") || !j.has("version") || !j.has("checksum")) {
            j.destroy();
            return ctx.jsonStatus("{\"error\":\"Missing required fields: name, version, checksum\",\"code\":\"BAD_REQUEST\"}", 400);
        }
        let name = j.get("name").asString();
        let version = j.get("version").asString();
        let checksum = String.lower(j.get("checksum").asString());
        let description = "";
        if (j.has("description")) {
            description = j.get("description").asString();
        }
        let license = "MIT";
        if (j.has("license")) {
            let lic = j.get("license").asString();
            if (lic != "") {
                license = lic;
            }
        }
        let archiveSizeBytes: int64 = 0;
        if (j.has("archiveSizeBytes")) {
            archiveSizeBytes = j.get("archiveSizeBytes").asInt();
        }
        j.destroy();

        if (!isValidPackageName(name)) {
            return ctx.jsonStatus("{\"error\":\"Invalid package name. Names must be 2-64 chars [a-z0-9_-].\",\"code\":\"INVALID_PACKAGE_NAME\"}", 400);
        }
        if (!isValidVersion(version)) {
            return ctx.jsonStatus("{\"error\":\"Invalid semver version format (expected X.Y.Z[-prerelease]).\",\"code\":\"INVALID_VERSION\"}", 400);
        }
        if (!isValidSha256(checksum)) {
            return ctx.jsonStatus("{\"error\":\"Invalid cryptographic checksum. Expected 64-character hexadecimal SHA-256 digest.\",\"code\":\"INVALID_CHECKSUM\"}", 400);
        }

        let dbPath = RegistryConfig.dbPath();
        let result = RegistryStore.publishVersion(dbPath, name, version, description, license, checksum, archiveSizeBytes);
        if (result == "EXISTS") {
            return ctx.jsonStatus("{\"error\":\"Package '" + escapeJson(name) + "@" + escapeJson(version) + "' has already been published. SP Registry releases are immutable and cannot be overwritten.\",\"code\":\"VERSION_ALREADY_EXISTS\"}", 409);
        }
        if (result == "DB_ERROR") {
            return ctx.jsonStatus("{\"error\":\"Registry storage error\",\"code\":\"STORAGE_ERROR\"}", 500);
        }
        let row = RegistryStore.findVersion(dbPath, name, version);
        let resp = "{\"success\":true,\"stored\":true,\"verified\":false,\"message\":\"Package metadata stored in the SP registry (SQLite). The checksum is stored as claimed: spm publish sends metadata only, no tarball bytes, so nothing has been verified against file bytes yet. Upload the tarball to enable verification.\",\"package\":" + row.toJson() + "}";
        return ctx.jsonStatus(resp, 201);
    }

    public static fn uploadTarball(Context ctx) -> HttpResponse {
        if (!RegistryAuth.validateBearer(ctx, RegistryConfig.token())) {
            return RegistryAuth.unauthorizedResponse(ctx);
        }
        let name = ctx.param("name");
        let version = ctx.param("version");
        let dbPath = RegistryConfig.dbPath();
        let row = RegistryStore.findVersion(dbPath, name, version);
        if (!row.found) {
            return ctx.jsonStatus("{\"error\":\"Package '" + escapeJson(name) + "@" + escapeJson(version) + "' not found. Publish its metadata first.\",\"code\":\"NOT_FOUND\"}", 404);
        }
        let body = ctx.body();
        if (body == "") {
            return ctx.jsonStatus("{\"error\":\"Empty request body: expected base64-encoded tarball bytes.\",\"code\":\"BAD_REQUEST\"}", 400);
        }
        if (!isValidBase64(body.trim())) {
            return ctx.jsonStatus("{\"error\":\"Request body is not valid base64.\",\"code\":\"BAD_REQUEST\"}", 400);
        }
        let b64 = body.trim();
        let bytes = Crypto.base64Decode(b64);
        let computed = Crypto.sha256(bytes);
        if (!Crypto.timingSafeEqual(computed, row.checksum)) {
            return ctx.jsonStatus("{\"error\":\"Tarball SHA-256 does not match the checksum stored at publish time.\",\"code\":\"CHECKSUM_MISMATCH\",\"expectedChecksum\":\"" + escapeJson(row.checksum) + "\",\"actualChecksum\":\"" + escapeJson(computed) + "\"}", 400);
        }
        let stored = RegistryStore.storeTarball(dbPath, name, version, b64, computed);
        if (stored == "DB_ERROR") {
            return ctx.jsonStatus("{\"error\":\"Registry storage error\",\"code\":\"STORAGE_ERROR\"}", 500);
        }
        if (stored == "NOT_FOUND") {
            return ctx.jsonStatus("{\"error\":\"Package '" + escapeJson(name) + "@" + escapeJson(version) + "' not found.\",\"code\":\"NOT_FOUND\"}", 404);
        }
        let resp = "{\"success\":true,\"stored\":true,\"verified\":true,\"name\":\"" + escapeJson(name) + "\",\"version\":\"" + escapeJson(version) + "\",\"sha256\":\"" + escapeJson(computed) + "\",\"message\":\"Tarball stored and its SHA-256 matches the published checksum.\"}";
        return ctx.json(resp);
    }

    public static fn verify(Context ctx) -> HttpResponse {
        let name = ctx.param("name");
        let version = ctx.param("version");
        let dbPath = RegistryConfig.dbPath();
        let status = RegistryStore.verifyStored(dbPath, name, version);
        if (status == "NOT_FOUND") {
            return ctx.jsonStatus("{\"error\":\"Package '" + escapeJson(name) + "' not found\",\"code\":\"NOT_FOUND\"}", 404);
        }
        if (status == "DB_ERROR") {
            return ctx.jsonStatus("{\"error\":\"Registry storage error\",\"code\":\"STORAGE_ERROR\"}", 500);
        }
        let row = RegistryStore.findVersion(dbPath, name, version);
        let verified = "false";
        let tarballStored = "false";
        let message = "No tarball has been uploaded for this version yet. The checksum is stored as claimed at publish time and has not been verified against file bytes.";
        if (status == "VERIFIED") {
            verified = "true";
            tarballStored = "true";
            message = "Verified: the stored tarball was decoded and its SHA-256 matches the checksum stored at publish time.";
        }
        if (status == "MISMATCH") {
            tarballStored = "true";
            message = "Verification failed: the stored tarball's SHA-256 does not match the checksum stored at publish time.";
        }
        let resp = "{\"name\":\"" + escapeJson(row.name) + "\",\"version\":\"" + escapeJson(row.version) + "\",\"checksum\":\"" + escapeJson(row.checksum) + "\",\"algorithm\":\"SHA-256\",\"verified\":" + verified + ",\"tarballStored\":" + tarballStored + ",\"message\":\"" + escapeJson(message) + "\"}";
        return ctx.json(resp);
    }

    public static fn packageInfo(Context ctx) -> HttpResponse {
        let name = ctx.param("name");
        let version = ctx.param("version");
        let row = RegistryStore.findVersion(RegistryConfig.dbPath(), name, version);
        if (!row.found) {
            return ctx.jsonStatus("{\"error\":\"Version '" + escapeJson(version) + "' of package '" + escapeJson(name) + "' not found\",\"code\":\"NOT_FOUND\"}", 404);
        }
        return ctx.json(row.toJson());
    }

    public static fn listPackages(Context ctx) -> HttpResponse {
        let listJson = RegistryStore.listPackagesJson(RegistryConfig.dbPath());
        let count = RegistryStore.countPackages(RegistryConfig.dbPath());
        let resp = "{\"packages\":" + listJson + ",\"count\":" + count + "}";
        return ctx.json(resp);
    }

    public static fn download(Context ctx) -> HttpResponse {
        let name = ctx.param("name");
        let version = ctx.param("version");
        let dbPath = RegistryConfig.dbPath();
        let row = RegistryStore.findVersion(dbPath, name, version);
        if (!row.found) {
            return ctx.jsonStatus("{\"error\":\"Version '" + escapeJson(version) + "' of package '" + escapeJson(name) + "' not found\",\"code\":\"NOT_FOUND\"}", 404);
        }
        if (!row.tarballStored) {
            return ctx.jsonStatus("{\"error\":\"No tarball has been uploaded for '" + escapeJson(name) + "@" + escapeJson(version) + "' yet.\",\"code\":\"TARBALL_NOT_UPLOADED\",\"hint\":\"Upload base64 tarball bytes with POST /api/v1/packages/" + escapeJson(name) + "/" + escapeJson(version) + "/tarball (Bearer token required).\"}", 404);
        }
        let b64 = RegistryStore.getTarballBase64(dbPath, name, version);
        let res = HttpResponse.create(200, b64);
        res.setHeader("Content-Type", "application/octet-stream");
        res.setHeader("X-Content-Encoding", "base64");
        res.setHeader("X-Sha256", row.checksum);
        return res;
    }

    public static fn openApi(Context ctx) -> HttpResponse {
        let path = RegistryConfig.openApiPath();
        if (!exists(path)) {
            return ctx.jsonStatus("{\"error\":\"OpenAPI spec file not found on the server. Start the service from examples/registry-backend/ or set REGISTRY_OPENAPI_PATH.\",\"code\":\"SPEC_NOT_FOUND\"}", 500);
        }
        let spec = readText(path);
        let res = HttpResponse.create(200, spec);
        res.setHeader("Content-Type", "application/yaml; charset=utf-8");
        return res;
    }
}
