module repositories.todo_repo;

import std.db;
import db.schema;
import models.todo;
import util.validation;

public class TodoFilter {
    public string status = "";
    public string priority = "";
    public string categoryId = "";
    public string search = "";
    public string dueState = "";
    public string sort = "created_at";
    public string order = "desc";
    public int64 page = 1;
    public int64 limit = 20;
}

public class TodoRepository {
    static fn readNullableString(SqliteStatement stmt, int idx) -> string {
        if (stmt.isNull(idx)) {
            return "";
        }
        return stmt.getString(idx);
    }

    static fn readRow(SqliteStatement stmt) -> Todo {
        let todo = new Todo();
        todo.id = stmt.getString(0);
        todo.userId = stmt.getString(1);
        todo.categoryId = TodoRepository.readNullableString(stmt, 2);
        todo.categoryName = TodoRepository.readNullableString(stmt, 3);
        todo.categoryColor = TodoRepository.readNullableString(stmt, 4);
        todo.title = stmt.getString(5);
        todo.description = stmt.getString(6);
        todo.status = stmt.getString(7);
        todo.priority = stmt.getString(8);
        todo.dueDate = TodoRepository.readNullableString(stmt, 9);
        todo.completedAt = TodoRepository.readNullableString(stmt, 10);
        todo.sortOrder = stmt.getInt(11);
        todo.createdAt = stmt.getString(12);
        todo.updatedAt = stmt.getString(13);
        todo.found = true;
        return todo;
    }

    static fn bindOptionalString(SqliteStatement stmt, int idx, string val) {
        let trimmed = val.trim();
        if (trimmed.length() == 0) {
            stmt.bindNull(idx);
        } else {
            stmt.bindString(idx, trimmed);
        }
    }

    static fn escapeLike(string raw) -> string {
        let out = "";
        let n = raw.length();
        let i = 0;
        while (i < n) {
            let ch = raw.charAt(i);
            if (ch == '\\' || ch == '%' || ch == '_') {
                out = out + "\\";
            }
            out = out + raw.substring(i, i + 1);
            i = i + 1;
        }
        return out;
    }

    static fn buildOrderClause(string sortField, string sortOrder) -> string {
        let dir = "DESC";
        if (sortOrder.lower() == "asc") {
            dir = "ASC";
        }
        if (sortField == "due_date") {
            return "ORDER BY CASE WHEN t.due_date IS NULL THEN 1 ELSE 0 END ASC, t.due_date " + dir + ", t.created_at DESC, t.rowid DESC";
        }
        if (sortField == "priority") {
            return "ORDER BY CASE t.priority WHEN 'urgent' THEN 1 WHEN 'high' THEN 2 WHEN 'medium' THEN 3 ELSE 4 END " + dir + ", t.created_at DESC, t.rowid DESC";
        }
        if (sortField == "title") {
            return "ORDER BY LOWER(t.title) " + dir + ", t.created_at DESC, t.rowid DESC";
        }
        if (sortField == "updated_at") {
            return "ORDER BY t.updated_at " + dir + ", t.rowid " + dir;
        }
        if (sortField == "sort_order") {
            return "ORDER BY t.sort_order " + dir + ", t.created_at DESC, t.rowid DESC";
        }
        return "ORDER BY t.created_at " + dir + ", t.rowid " + dir;
    }

    static fn bindStringList(SqliteStatement stmt, List<string> values) -> int {
        let i = 0;
        let count = values.length();
        while (i < count) {
            stmt.bindString(i + 1, values.get(i));
            i = i + 1;
        }
        return count + 1;
    }

    public static fn listByUser(string userId, TodoFilter filter) -> PaginatedTodos {
        let result = new PaginatedTodos();
        let page = filter.page;
        if (page < 1) {
            page = 1;
        }
        let limit = filter.limit;
        if (limit < 1) {
            limit = 20;
        }
        if (limit > 100) {
            limit = 100;
        }
        result.page = page;
        result.limit = limit;

        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return result;
        }
        let today = DatabaseManager.utcToday(db);

        let whereSql = "WHERE t.user_id = ?";
        let bindParams = new List<string>();
        bindParams.push(userId);

        let status = filter.status.trim().lower();
        if (status == "active") {
            whereSql = whereSql + " AND t.status IN ('todo', 'in_progress')";
        } else if (status.length() > 0 && status != "all" && ValidationUtil.isValidStatus(status)) {
            whereSql = whereSql + " AND t.status = ?";
            bindParams.push(status);
        }

        let priority = filter.priority.trim().lower();
        if (priority.length() > 0 && priority != "all" && ValidationUtil.isValidPriority(priority)) {
            whereSql = whereSql + " AND t.priority = ?";
            bindParams.push(priority);
        }

        let catId = filter.categoryId.trim();
        if (catId == "uncategorized") {
            whereSql = whereSql + " AND t.category_id IS NULL";
        } else if (catId.length() > 0 && catId != "all") {
            whereSql = whereSql + " AND t.category_id = ?";
            bindParams.push(catId);
        }

        let due = filter.dueState.trim().lower();
        if (due == "overdue" && today.length() > 0) {
            whereSql = whereSql + " AND t.due_date IS NOT NULL AND SUBSTR(t.due_date, 1, 10) < ? AND t.status IN ('todo', 'in_progress')";
            bindParams.push(today);
        } else if (due == "today" && today.length() > 0) {
            whereSql = whereSql + " AND t.due_date IS NOT NULL AND SUBSTR(t.due_date, 1, 10) = ?";
            bindParams.push(today);
        } else if (due == "upcoming" && today.length() > 0) {
            whereSql = whereSql + " AND t.due_date IS NOT NULL AND SUBSTR(t.due_date, 1, 10) > ? AND t.status IN ('todo', 'in_progress')";
            bindParams.push(today);
        } else if (due == "none") {
            whereSql = whereSql + " AND t.due_date IS NULL";
        }

        let q = filter.search.trim();
        if (q.length() > 0) {
            let pattern = "%" + TodoRepository.escapeLike(q) + "%";
            whereSql = whereSql + " AND (LOWER(t.title) LIKE LOWER(?) ESCAPE '\\' OR LOWER(t.description) LIKE LOWER(?) ESCAPE '\\')";
            bindParams.push(pattern);
            bindParams.push(pattern);
        }

        let countSql = "SELECT COUNT(*) FROM todos t " + whereSql + ";";
        let countStmt = db.prepare(countSql);
        if (!countStmt.isValid()) {
            db.close();
            return result;
        }
        TodoRepository.bindStringList(countStmt, bindParams);
        let total: int64 = 0;
        if (countStmt.next()) {
            total = countStmt.getInt(0);
        }
        countStmt.finalize();

        result.total = total;
        let totalPages: int64 = 1;
        if (total > 0) {
            totalPages = (total + limit - 1) / limit;
        }
        result.totalPages = totalPages;
        result.hasNext = page < totalPages;
        result.hasPrev = page > 1;

        let offset: int64 = (page - 1) * limit;
        let orderSql = TodoRepository.buildOrderClause(filter.sort.trim().lower(), filter.order);
        let dataSql = "SELECT t.id, t.user_id, t.category_id, c.name, c.color, "
            + "t.title, t.description, t.status, t.priority, "
            + "t.due_date, t.completed_at, t.sort_order, "
            + "t.created_at, t.updated_at "
            + "FROM todos t "
            + "LEFT JOIN categories c ON c.id = t.category_id AND c.user_id = t.user_id "
            + whereSql + " " + orderSql + " LIMIT ? OFFSET ?;";

        let dataStmt = db.prepare(dataSql);
        if (!dataStmt.isValid()) {
            db.close();
            return result;
        }
        let nextIdx = TodoRepository.bindStringList(dataStmt, bindParams);
        dataStmt.bindInt(nextIdx, limit);
        dataStmt.bindInt(nextIdx + 1, offset);

        let arr = "[";
        let idx = 0;
        while (dataStmt.next()) {
            if (idx > 0) {
                arr = arr + ",";
            }
            arr = arr + TodoRepository.readRow(dataStmt).toJson();
            idx = idx + 1;
        }
        arr = arr + "]";
        dataStmt.finalize();
        db.close();

        result.itemsJson = arr;
        return result;
    }

    public static fn findById(string todoId) -> Todo {
        let empty = new Todo();
        if (todoId.trim().length() == 0) {
            return empty;
        }
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return empty;
        }
        let stmt = db.prepare(
            "SELECT t.id, t.user_id, t.category_id, c.name, c.color, "
            + "t.title, t.description, t.status, t.priority, "
            + "t.due_date, t.completed_at, t.sort_order, "
            + "t.created_at, t.updated_at "
            + "FROM todos t "
            + "LEFT JOIN categories c ON c.id = t.category_id AND c.user_id = t.user_id "
            + "WHERE t.id = ? LIMIT 1;"
        );
        if (!stmt.isValid()) {
            db.close();
            return empty;
        }
        stmt.bindString(1, todoId);
        let result = empty;
        if (stmt.next()) {
            result = TodoRepository.readRow(stmt);
        }
        stmt.finalize();
        db.close();
        return result;
    }

    public static fn create(
        string id,
        string userId,
        string categoryId,
        string title,
        string description,
        string status,
        string priority,
        string dueDate,
        string completedAt,
        int64 sortOrder
    ) -> Todo {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return new Todo();
        }
        let now = DatabaseManager.utcNow(db);
        let stmt = db.prepare(
            "INSERT INTO todos ("
            + "id, user_id, category_id, title, description, status, priority, "
            + "due_date, completed_at, sort_order, created_at, updated_at"
            + ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
        );
        if (!stmt.isValid()) {
            db.close();
            return new Todo();
        }
        stmt.bindString(1, id);
        stmt.bindString(2, userId);
        TodoRepository.bindOptionalString(stmt, 3, categoryId);
        stmt.bindString(4, title.trim());
        stmt.bindString(5, description);
        stmt.bindString(6, status);
        stmt.bindString(7, priority);
        TodoRepository.bindOptionalString(stmt, 8, dueDate);
        TodoRepository.bindOptionalString(stmt, 9, completedAt);
        stmt.bindInt(10, sortOrder);
        stmt.bindString(11, now);
        stmt.bindString(12, now);

        let rc = stmt.step();
        stmt.finalize();
        db.close();
        if (rc != 101) {
            return new Todo();
        }
        return TodoRepository.findById(id);
    }

    public static fn update(
        string todoId,
        string userId,
        string categoryId,
        string title,
        string description,
        string status,
        string priority,
        string dueDate,
        string completedAt,
        int64 sortOrder
    ) -> Todo {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return new Todo();
        }
        let now = DatabaseManager.utcNow(db);
        let stmt = db.prepare(
            "UPDATE todos SET "
            + "category_id = ?, title = ?, description = ?, status = ?, priority = ?, "
            + "due_date = ?, completed_at = ?, sort_order = ?, updated_at = ? "
            + "WHERE id = ? AND user_id = ?;"
        );
        if (!stmt.isValid()) {
            db.close();
            return new Todo();
        }
        TodoRepository.bindOptionalString(stmt, 1, categoryId);
        stmt.bindString(2, title.trim());
        stmt.bindString(3, description);
        stmt.bindString(4, status);
        stmt.bindString(5, priority);
        TodoRepository.bindOptionalString(stmt, 6, dueDate);
        TodoRepository.bindOptionalString(stmt, 7, completedAt);
        stmt.bindInt(8, sortOrder);
        stmt.bindString(9, now);
        stmt.bindString(10, todoId);
        stmt.bindString(11, userId);

        let rc = stmt.step();
        stmt.finalize();
        db.close();
        if (rc != 101) {
            return new Todo();
        }
        return TodoRepository.findById(todoId);
    }

    public static fn deleteById(string todoId, string userId) -> bool {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return false;
        }
        let stmt = db.prepare("DELETE FROM todos WHERE id = ? AND user_id = ?;");
        if (!stmt.isValid()) {
            db.close();
            return false;
        }
        stmt.bindString(1, todoId);
        stmt.bindString(2, userId);
        let rc = stmt.step();
        stmt.finalize();
        let affected = db.changes();
        db.close();
        return rc == 101 && affected > 0;
    }

    public static fn getStats(string userId) -> TodoStats {
        let stats = new TodoStats();
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return stats;
        }
        let today = DatabaseManager.utcToday(db);
        let stmt = db.prepare(
            "SELECT "
            + "COUNT(*) AS total, "
            + "COALESCE(SUM(CASE WHEN status = 'todo' THEN 1 ELSE 0 END), 0) AS todo_cnt, "
            + "COALESCE(SUM(CASE WHEN status = 'in_progress' THEN 1 ELSE 0 END), 0) AS prog_cnt, "
            + "COALESCE(SUM(CASE WHEN status = 'done' THEN 1 ELSE 0 END), 0) AS done_cnt, "
            + "COALESCE(SUM(CASE WHEN status = 'archived' THEN 1 ELSE 0 END), 0) AS arch_cnt, "
            + "COALESCE(SUM(CASE WHEN due_date IS NOT NULL AND SUBSTR(due_date, 1, 10) < ? AND status IN ('todo', 'in_progress') THEN 1 ELSE 0 END), 0) AS overdue_cnt, "
            + "COALESCE(SUM(CASE WHEN due_date IS NOT NULL AND SUBSTR(due_date, 1, 10) = ? AND status IN ('todo', 'in_progress') THEN 1 ELSE 0 END), 0) AS today_cnt, "
            + "COALESCE(SUM(CASE WHEN priority = 'urgent' AND status IN ('todo', 'in_progress') THEN 1 ELSE 0 END), 0) AS urgent_cnt, "
            + "COALESCE(SUM(CASE WHEN priority = 'high' AND status IN ('todo', 'in_progress') THEN 1 ELSE 0 END), 0) AS high_cnt "
            + "FROM todos WHERE user_id = ?;"
        );
        if (!stmt.isValid()) {
            db.close();
            return stats;
        }
        stmt.bindString(1, today);
        stmt.bindString(2, today);
        stmt.bindString(3, userId);
        if (stmt.next()) {
            stats.total = stmt.getInt(0);
            stats.todoCount = stmt.getInt(1);
            stats.inProgressCount = stmt.getInt(2);
            stats.doneCount = stmt.getInt(3);
            stats.archivedCount = stmt.getInt(4);
            stats.overdueCount = stmt.getInt(5);
            stats.dueTodayCount = stmt.getInt(6);
            stats.urgentCount = stmt.getInt(7);
            stats.highCount = stmt.getInt(8);
        }
        stmt.finalize();
        db.close();
        return stats;
    }

    public static fn bulkExecute(
        string userId,
        List<string> ids,
        string action,
        string priority
    ) -> int64 {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return 0;
        }
        let now = DatabaseManager.utcNow(db);
        db.beginTransaction();

        let affected: int64 = 0;
        let i = 0;
        let count = ids.length();
        while (i < count) {
            let todoId = ids.get(i).trim();
            i = i + 1;
            if (!ValidationUtil.isValidUuid(todoId)) {
                continue;
            }
            if (action == "delete") {
                let stmt = db.prepare("DELETE FROM todos WHERE id = ? AND user_id = ?;");
                if (stmt.isValid()) {
                    stmt.bindString(1, todoId);
                    stmt.bindString(2, userId);
                    if (stmt.step() == 101) {
                        affected = affected + db.changes();
                    }
                    stmt.finalize();
                }
            } else if (action == "complete") {
                let stmt = db.prepare(
                    "UPDATE todos SET status = 'done', completed_at = COALESCE(completed_at, ?), updated_at = ? "
                    + "WHERE id = ? AND user_id = ?;"
                );
                if (stmt.isValid()) {
                    stmt.bindString(1, now);
                    stmt.bindString(2, now);
                    stmt.bindString(3, todoId);
                    stmt.bindString(4, userId);
                    if (stmt.step() == 101) {
                        affected = affected + db.changes();
                    }
                    stmt.finalize();
                }
            } else if (action == "reopen") {
                let stmt = db.prepare(
                    "UPDATE todos SET status = 'todo', completed_at = NULL, updated_at = ? "
                    + "WHERE id = ? AND user_id = ?;"
                );
                if (stmt.isValid()) {
                    stmt.bindString(1, now);
                    stmt.bindString(2, todoId);
                    stmt.bindString(3, userId);
                    if (stmt.step() == 101) {
                        affected = affected + db.changes();
                    }
                    stmt.finalize();
                }
            } else if (action == "archive") {
                let stmt = db.prepare(
                    "UPDATE todos SET status = 'archived', updated_at = ? "
                    + "WHERE id = ? AND user_id = ?;"
                );
                if (stmt.isValid()) {
                    stmt.bindString(1, now);
                    stmt.bindString(2, todoId);
                    stmt.bindString(3, userId);
                    if (stmt.step() == 101) {
                        affected = affected + db.changes();
                    }
                    stmt.finalize();
                }
            } else if (action == "set_priority") {
                let stmt = db.prepare(
                    "UPDATE todos SET priority = ?, updated_at = ? "
                    + "WHERE id = ? AND user_id = ?;"
                );
                if (stmt.isValid()) {
                    stmt.bindString(1, priority);
                    stmt.bindString(2, now);
                    stmt.bindString(3, todoId);
                    stmt.bindString(4, userId);
                    if (stmt.step() == 101) {
                        affected = affected + db.changes();
                    }
                    stmt.finalize();
                }
            }
        }

        db.commit();
        db.close();
        return affected;
    }
}
