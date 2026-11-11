module repositories.user_repo;

import std.db;
import db.schema;
import models.user;

public class UserRepository {
    static fn readRow(SqliteStatement stmt) -> User {
        let user = new User();
        user.id = stmt.getString(0);
        user.email = stmt.getString(1);
        user.name = stmt.getString(2);
        user.passwordHash = stmt.getString(3);
        user.createdAt = stmt.getString(4);
        user.updatedAt = stmt.getString(5);
        user.found = true;
        return user;
    }

    public static fn findByEmail(string email) -> User {
        let normalized = email.trim().lower();
        let empty = new User();
        if (normalized.length() == 0) {
            return empty;
        }
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return empty;
        }
        let stmt = db.prepare(
            "SELECT id, email, name, password_hash, created_at, updated_at FROM users WHERE email = ? LIMIT 1;"
        );
        if (!stmt.isValid()) {
            db.close();
            return empty;
        }
        stmt.bindString(1, normalized);
        let result = empty;
        if (stmt.next()) {
            result = UserRepository.readRow(stmt);
        }
        stmt.finalize();
        db.close();
        return result;
    }

    public static fn findById(string userId) -> User {
        let trimmed = userId.trim();
        let empty = new User();
        if (trimmed.length() == 0) {
            return empty;
        }
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return empty;
        }
        let stmt = db.prepare(
            "SELECT id, email, name, password_hash, created_at, updated_at FROM users WHERE id = ? LIMIT 1;"
        );
        if (!stmt.isValid()) {
            db.close();
            return empty;
        }
        stmt.bindString(1, trimmed);
        let result = empty;
        if (stmt.next()) {
            result = UserRepository.readRow(stmt);
        }
        stmt.finalize();
        db.close();
        return result;
    }

    public static fn createUser(string id, string email, string name, string passwordHash) -> User {
        let empty = new User();
        let normalizedEmail = email.trim().lower();
        let cleanName = name.trim();
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return empty;
        }
        let now = DatabaseManager.utcNow(db);
        let stmt = db.prepare(
            "INSERT INTO users (id, email, name, password_hash, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?);"
        );
        if (!stmt.isValid()) {
            db.close();
            return empty;
        }
        stmt.bindString(1, id);
        stmt.bindString(2, normalizedEmail);
        stmt.bindString(3, cleanName);
        stmt.bindString(4, passwordHash);
        stmt.bindString(5, now);
        stmt.bindString(6, now);

        let stepCode = stmt.step();
        stmt.finalize();
        db.close();

        if (stepCode != 101) {
            return empty;
        }

        let created = new User();
        created.id = id;
        created.email = normalizedEmail;
        created.name = cleanName;
        created.passwordHash = passwordHash;
        created.createdAt = now;
        created.updatedAt = now;
        created.found = true;
        return created;
    }

    public static fn updateName(string userId, string name) -> User {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return new User();
        }
        let now = DatabaseManager.utcNow(db);
        let stmt = db.prepare("UPDATE users SET name = ?, updated_at = ? WHERE id = ?;");
        if (!stmt.isValid()) {
            db.close();
            return new User();
        }
        stmt.bindString(1, name.trim());
        stmt.bindString(2, now);
        stmt.bindString(3, userId);
        let rc = stmt.step();
        stmt.finalize();
        db.close();
        if (rc != 101) {
            return new User();
        }
        return UserRepository.findById(userId);
    }

    public static fn countUsers() -> int64 {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return 0;
        }
        let stmt = db.prepare("SELECT COUNT(*) FROM users;");
        if (!stmt.isValid()) {
            db.close();
            return 0;
        }
        let total: int64 = 0;
        if (stmt.next()) {
            total = stmt.getInt(0);
        }
        stmt.finalize();
        db.close();
        return total;
    }
}
