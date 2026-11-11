module repositories.category_repo;

import std.db;
import std.crypto;
import db.schema;
import models.category;

public class CategoryRepository {
    static fn readRow(SqliteStatement stmt) -> Category {
        let cat = new Category();
        cat.id = stmt.getString(0);
        cat.userId = stmt.getString(1);
        cat.name = stmt.getString(2);
        cat.color = stmt.getString(3);
        cat.sortOrder = stmt.getInt(4);
        cat.todoCount = stmt.getInt(5);
        cat.createdAt = stmt.getString(6);
        cat.updatedAt = stmt.getString(7);
        cat.found = true;
        return cat;
    }

    public static fn listByUser(string userId) -> List<Category> {
        let items = new List<Category>();
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return items;
        }
        let stmt = db.prepare(
            "SELECT c.id, c.user_id, c.name, c.color, c.sort_order, "
            + "(SELECT COUNT(*) FROM todos t WHERE t.category_id = c.id AND t.user_id = c.user_id) AS todo_count, "
            + "c.created_at, c.updated_at "
            + "FROM categories c WHERE c.user_id = ? "
            + "ORDER BY c.sort_order ASC, c.name ASC;"
        );
        if (!stmt.isValid()) {
            db.close();
            return items;
        }
        stmt.bindString(1, userId);
        while (stmt.next()) {
            items.push(CategoryRepository.readRow(stmt));
        }
        stmt.finalize();
        db.close();
        return items;
    }

    public static fn findById(string categoryId) -> Category {
        let empty = new Category();
        if (categoryId.trim().length() == 0) {
            return empty;
        }
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return empty;
        }
        let stmt = db.prepare(
            "SELECT c.id, c.user_id, c.name, c.color, c.sort_order, "
            + "(SELECT COUNT(*) FROM todos t WHERE t.category_id = c.id AND t.user_id = c.user_id) AS todo_count, "
            + "c.created_at, c.updated_at "
            + "FROM categories c WHERE c.id = ? LIMIT 1;"
        );
        if (!stmt.isValid()) {
            db.close();
            return empty;
        }
        stmt.bindString(1, categoryId);
        let result = empty;
        if (stmt.next()) {
            result = CategoryRepository.readRow(stmt);
        }
        stmt.finalize();
        db.close();
        return result;
    }

    public static fn existsNameForUser(string userId, string name, string excludeCategoryId = "") -> bool {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return false;
        }
        let sql = "SELECT COUNT(*) FROM categories WHERE user_id = ? AND LOWER(name) = LOWER(?)";
        if (excludeCategoryId.length() > 0) {
            sql = sql + " AND id != ?;";
        } else {
            sql = sql + ";";
        }
        let stmt = db.prepare(sql);
        if (!stmt.isValid()) {
            db.close();
            return false;
        }
        stmt.bindString(1, userId);
        stmt.bindString(2, name.trim());
        if (excludeCategoryId.length() > 0) {
            stmt.bindString(3, excludeCategoryId);
        }
        let count: int64 = 0;
        if (stmt.next()) {
            count = stmt.getInt(0);
        }
        stmt.finalize();
        db.close();
        return count > 0;
    }

    public static fn create(
        string id,
        string userId,
        string name,
        string color,
        int64 sortOrder
    ) -> Category {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return new Category();
        }
        let now = DatabaseManager.utcNow(db);
        let stmt = db.prepare(
            "INSERT INTO categories (id, user_id, name, color, sort_order, created_at, updated_at) "
            + "VALUES (?, ?, ?, ?, ?, ?, ?);"
        );
        if (!stmt.isValid()) {
            db.close();
            return new Category();
        }
        let cleanName = name.trim();
        let cleanColor = color.trim();
        stmt.bindString(1, id);
        stmt.bindString(2, userId);
        stmt.bindString(3, cleanName);
        stmt.bindString(4, cleanColor);
        stmt.bindInt(5, sortOrder);
        stmt.bindString(6, now);
        stmt.bindString(7, now);

        let rc = stmt.step();
        stmt.finalize();
        db.close();
        if (rc != 101) {
            return new Category();
        }
        return CategoryRepository.findById(id);
    }

    public static fn update(
        string categoryId,
        string userId,
        string name,
        string color,
        int64 sortOrder
    ) -> Category {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return new Category();
        }
        let now = DatabaseManager.utcNow(db);
        let stmt = db.prepare(
            "UPDATE categories SET name = ?, color = ?, sort_order = ?, updated_at = ? "
            + "WHERE id = ? AND user_id = ?;"
        );
        if (!stmt.isValid()) {
            db.close();
            return new Category();
        }
        stmt.bindString(1, name.trim());
        stmt.bindString(2, color.trim());
        stmt.bindInt(3, sortOrder);
        stmt.bindString(4, now);
        stmt.bindString(5, categoryId);
        stmt.bindString(6, userId);

        let rc = stmt.step();
        stmt.finalize();
        db.close();
        if (rc != 101) {
            return new Category();
        }
        return CategoryRepository.findById(categoryId);
    }

    public static fn deleteById(string categoryId, string userId) -> bool {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return false;
        }
        let stmt = db.prepare("DELETE FROM categories WHERE id = ? AND user_id = ?;");
        if (!stmt.isValid()) {
            db.close();
            return false;
        }
        stmt.bindString(1, categoryId);
        stmt.bindString(2, userId);
        let rc = stmt.step();
        stmt.finalize();
        let affected = db.changes();
        db.close();
        return rc == 101 && affected > 0;
    }

    public static fn seedDefaults(string userId) {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return;
        }
        let now = DatabaseManager.utcNow(db);
        db.beginTransaction();

        CategoryRepository.insertSeedRow(db, Crypto.uuid(), userId, "Work", "#3B82F6", 1, now);
        CategoryRepository.insertSeedRow(db, Crypto.uuid(), userId, "Personal", "#10B981", 2, now);
        CategoryRepository.insertSeedRow(db, Crypto.uuid(), userId, "Errands", "#F59E0B", 3, now);

        db.commit();
        db.close();
    }

    static fn insertSeedRow(
        SqliteConnection db,
        string id,
        string userId,
        string name,
        string color,
        int64 sortOrder,
        string now
    ) {
        let stmt = db.prepare(
            "INSERT OR IGNORE INTO categories (id, user_id, name, color, sort_order, created_at, updated_at) "
            + "VALUES (?, ?, ?, ?, ?, ?, ?);"
        );
        if (!stmt.isValid()) {
            return;
        }
        stmt.bindString(1, id);
        stmt.bindString(2, userId);
        stmt.bindString(3, name);
        stmt.bindString(4, color);
        stmt.bindInt(5, sortOrder);
        stmt.bindString(6, now);
        stmt.bindString(7, now);
        stmt.step();
        stmt.finalize();
    }
}
