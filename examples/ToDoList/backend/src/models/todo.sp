module models.todo;

import util.json;

public class Todo {
    public string id = "";
    public string userId = "";
    public string categoryId = "";
    public string categoryName = "";
    public string categoryColor = "";
    public string title = "";
    public string description = "";
    public string status = "todo";
    public string priority = "medium";
    public string dueDate = "";
    public string completedAt = "";
    public int64 sortOrder = 0;
    public string createdAt = "";
    public string updatedAt = "";
    public bool found = false;

    public fn toJson() -> string {
        let catObj = "null";
        if (this.categoryId.length() > 0 && this.categoryName.length() > 0) {
            catObj = "{\"id\":" + JsonUtil.quote(this.categoryId)
                + ",\"name\":" + JsonUtil.quote(this.categoryName)
                + ",\"color\":" + JsonUtil.quote(this.categoryColor) + "}";
        }

        return "{\"id\":" + JsonUtil.quote(this.id)
            + ",\"user_id\":" + JsonUtil.quote(this.userId)
            + ",\"category_id\":" + JsonUtil.nullableString(this.categoryId)
            + ",\"category\":" + catObj
            + ",\"title\":" + JsonUtil.quote(this.title)
            + ",\"description\":" + JsonUtil.quote(this.description)
            + ",\"status\":" + JsonUtil.quote(this.status)
            + ",\"priority\":" + JsonUtil.quote(this.priority)
            + ",\"due_date\":" + JsonUtil.nullableString(this.dueDate)
            + ",\"completed_at\":" + JsonUtil.nullableString(this.completedAt)
            + ",\"sort_order\":${this.sortOrder}"
            + ",\"created_at\":" + JsonUtil.quote(this.createdAt)
            + ",\"updated_at\":" + JsonUtil.quote(this.updatedAt)
            + "}";
    }
}

public class TodoStats {
    public int64 total = 0;
    public int64 todoCount = 0;
    public int64 inProgressCount = 0;
    public int64 doneCount = 0;
    public int64 archivedCount = 0;
    public int64 overdueCount = 0;
    public int64 dueTodayCount = 0;
    public int64 urgentCount = 0;
    public int64 highCount = 0;

    public fn toJson() -> string {
        return "{\"total\":${this.total}"
            + ",\"todo\":${this.todoCount}"
            + ",\"in_progress\":${this.inProgressCount}"
            + ",\"done\":${this.doneCount}"
            + ",\"archived\":${this.archivedCount}"
            + ",\"overdue\":${this.overdueCount}"
            + ",\"due_today\":${this.dueTodayCount}"
            + ",\"urgent\":${this.urgentCount}"
            + ",\"high\":${this.highCount}"
            + "}";
    }
}

public class PaginatedTodos {
    public string itemsJson = "[]";
    public int64 total = 0;
    public int64 page = 1;
    public int64 limit = 20;
    public int64 totalPages = 1;
    public bool hasNext = false;
    public bool hasPrev = false;

    public fn toJson() -> string {
        return "{\"items\":" + this.itemsJson
            + ",\"meta\":{"
            + "\"total\":${this.total}"
            + ",\"page\":${this.page}"
            + ",\"limit\":${this.limit}"
            + ",\"total_pages\":${this.totalPages}"
            + ",\"has_next\":" + JsonUtil.boolLiteral(this.hasNext)
            + ",\"has_prev\":" + JsonUtil.boolLiteral(this.hasPrev)
            + "}}";
    }
}
