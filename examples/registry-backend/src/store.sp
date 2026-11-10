module store;

import std.db;
import std.crypto;
import json_util;

// ============================================================================
// SQLite storage for the registry service.
//
// One table, one row per published package version. Releases are immutable:
// publishVersion refuses a name@version that already exists (409 upstream).
//
// Tarball bytes: the SP SQLite API has no BLOB binding, and the runtime
// reads TEXT columns back through a C string, so raw tarball bytes are
// NEVER stored as a blob. The tarball is stored as base64 TEXT instead
// (tarball_base64) and hashed in memory only. Download returns the same
// base64 text with X-Content-Encoding: base64 so clients can decode it.
// ============================================================================

public class PackageRow {
    public string name;
    public string version;
    public string description;
    public string license;
    public string checksum;
    public int64 archiveSizeBytes;
    public string publishedAt;
    public bool tarballStored;
    public bool found;

    public fn toJson() -> string {
        let stored = "false";
        if (this.tarballStored) {
            stored = "true";
        }
        return "{\"name\":\"" + escapeJson(this.name) + "\",\"version\":\"" + escapeJson(this.version) + "\",\"description\":\"" + escapeJson(this.description) + "\",\"license\":\"" + escapeJson(this.license) + "\",\"checksum\":\"" + escapeJson(this.checksum) + "\",\"archiveSizeBytes\":" + this.archiveSizeBytes + ",\"publishedAt\":\"" + escapeJson(this.publishedAt) + "\",\"tarballStored\":" + stored + ",\"downloadUrl\":\"/api/v1/packages/" + escapeJson(this.name) + "/" + escapeJson(this.version) + "/download\"}";
    }
}

public class RegistryStore {
    public static fn initSchema(string dbPath) -> bool {
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return false;
        }
        let rc = db.execute("CREATE TABLE IF NOT EXISTS packages (name TEXT NOT NULL, version TEXT NOT NULL, description TEXT NOT NULL DEFAULT '', license TEXT NOT NULL DEFAULT 'MIT', checksum TEXT NOT NULL, archive_size_bytes INTEGER NOT NULL DEFAULT 0, tarball_base64 TEXT, tarball_sha256 TEXT, published_at TEXT NOT NULL DEFAULT (datetime('now')), PRIMARY KEY (name, version));");
        db.close();
        return rc >= 0;
    }

    // Returns "" on success, "EXISTS" when name@version is already stored,
    // "DB_ERROR" when SQLite fails.
    public static fn publishVersion(string dbPath, string name, string version, string description, string license, string checksum, int64 archiveSizeBytes) -> string {
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return "DB_ERROR";
        }
        let check = db.prepare("SELECT COUNT(*) FROM packages WHERE name = ?1 AND version = ?2;");
        check.bindString(1, name);
        check.bindString(2, version);
        let exists = false;
        if (check.next()) {
            if (check.getInt(0) > 0) {
                exists = true;
            }
        }
        check.finalize();
        if (exists) {
            db.close();
            return "EXISTS";
        }
        let ins = db.prepare("INSERT INTO packages (name, version, description, license, checksum, archive_size_bytes) VALUES (?1, ?2, ?3, ?4, ?5, ?6);");
        ins.bindString(1, name);
        ins.bindString(2, version);
        ins.bindString(3, description);
        ins.bindString(4, license);
        ins.bindString(5, checksum);
        ins.bindInt(6, archiveSizeBytes);
        let stepped = ins.step();
        ins.finalize();
        db.close();
        if (stepped == 101) {
            return "";
        }
        return "DB_ERROR";
    }

    public static fn findVersion(string dbPath, string name, string version) -> PackageRow {
        let row = new PackageRow();
        row.found = false;
        row.name = name;
        row.version = version;
        row.description = "";
        row.license = "MIT";
        row.checksum = "";
        row.archiveSizeBytes = 0;
        row.publishedAt = "";
        row.tarballStored = false;
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return row;
        }
        let stmt = db.prepare("SELECT name, version, description, license, checksum, archive_size_bytes, published_at, (tarball_base64 IS NOT NULL) FROM packages WHERE name = ?1 AND version = ?2;");
        stmt.bindString(1, name);
        stmt.bindString(2, version);
        if (stmt.next()) {
            row.found = true;
            row.name = stmt.getString(0);
            row.version = stmt.getString(1);
            row.description = stmt.getString(2);
            row.license = stmt.getString(3);
            row.checksum = stmt.getString(4);
            row.archiveSizeBytes = stmt.getInt(5);
            row.publishedAt = stmt.getString(6);
            row.tarballStored = stmt.getInt(7) == 1;
        }
        stmt.finalize();
        db.close();
        return row;
    }

    public static fn listPackagesJson(string dbPath) -> string {
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return "[]";
        }
        let stmt = db.prepare("SELECT name, version, description, license, checksum, archive_size_bytes, published_at, (tarball_base64 IS NOT NULL) FROM packages ORDER BY name ASC, version ASC;");
        let out = "[";
        let first = true;
        while (stmt.next()) {
            let row = new PackageRow();
            row.found = true;
            row.name = stmt.getString(0);
            row.version = stmt.getString(1);
            row.description = stmt.getString(2);
            row.license = stmt.getString(3);
            row.checksum = stmt.getString(4);
            row.archiveSizeBytes = stmt.getInt(5);
            row.publishedAt = stmt.getString(6);
            row.tarballStored = stmt.getInt(7) == 1;
            if (!first) {
                out = out + ",";
            }
            first = false;
            out = out + row.toJson();
        }
        stmt.finalize();
        db.close();
        out = out + "]";
        return out;
    }

    public static fn countPackages(string dbPath) -> int64 {
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return 0;
        }
        let stmt = db.prepare("SELECT COUNT(*) FROM packages;");
        let count: int64 = 0;
        if (stmt.next()) {
            count = stmt.getInt(0);
        }
        stmt.finalize();
        db.close();
        return count;
    }

    // Stores a base64 tarball for an already-published version.
    // Returns "OK", "NOT_FOUND" (no such name@version), or "DB_ERROR".
    public static fn storeTarball(string dbPath, string name, string version, string tarballBase64, string tarballSha256) -> string {
        let existing = RegistryStore.findVersion(dbPath, name, version);
        if (!existing.found) {
            return "NOT_FOUND";
        }
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return "DB_ERROR";
        }
        let upd = db.prepare("UPDATE packages SET tarball_base64 = ?1, tarball_sha256 = ?2 WHERE name = ?3 AND version = ?4;");
        upd.bindString(1, tarballBase64);
        upd.bindString(2, tarballSha256);
        upd.bindString(3, name);
        upd.bindString(4, version);
        let stepped = upd.step();
        upd.finalize();
        db.close();
        if (stepped == 101) {
            return "OK";
        }
        return "DB_ERROR";
    }

    public static fn getTarballBase64(string dbPath, string name, string version) -> string {
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return "";
        }
        let stmt = db.prepare("SELECT tarball_base64 FROM packages WHERE name = ?1 AND version = ?2;");
        stmt.bindString(1, name);
        stmt.bindString(2, version);
        let b64 = "";
        if (stmt.next()) {
            if (!stmt.isNull(0)) {
                b64 = stmt.getString(0);
            }
        }
        stmt.finalize();
        db.close();
        return b64;
    }

    // Real verification, computed at request time from stored data:
    // decode the stored base64 tarball, SHA-256 it, and compare against the
    // checksum stored at publish time AND the hash stored at upload time.
    // Returns "VERIFIED", "NO_TARBALL" (published, but no tarball uploaded
    // yet, so the checksum is stored-as-claimed only), "MISMATCH",
    // "NOT_FOUND", or "DB_ERROR".
    public static fn verifyStored(string dbPath, string name, string version) -> string {
        let db = Database.openSqlite(dbPath, 5000, true);
        if (!db.isOpen()) {
            return "DB_ERROR";
        }
        let stmt = db.prepare("SELECT checksum, tarball_base64, tarball_sha256 FROM packages WHERE name = ?1 AND version = ?2;");
        stmt.bindString(1, name);
        stmt.bindString(2, version);
        if (!stmt.next()) {
            stmt.finalize();
            db.close();
            return "NOT_FOUND";
        }
        let checksum = stmt.getString(0);
        if (stmt.isNull(1)) {
            stmt.finalize();
            db.close();
            return "NO_TARBALL";
        }
        let b64 = stmt.getString(1);
        let storedHash = stmt.getString(2);
        stmt.finalize();
        db.close();
        let bytes = Crypto.base64Decode(b64);
        let computed = Crypto.sha256(bytes);
        if (Crypto.timingSafeEqual(computed, checksum) && Crypto.timingSafeEqual(computed, storedHash)) {
            return "VERIFIED";
        }
        return "MISMATCH";
    }
}
