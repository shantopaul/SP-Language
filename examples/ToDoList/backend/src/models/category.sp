module models.category;

import util.json;

public class Category {
    public string id = "";
    public string userId = "";
    public string name = "";
    public string color = "#3B82F6";
    public int64 sortOrder = 0;
    public int64 todoCount = 0;
    public string createdAt = "";
    public string updatedAt = "";
    public bool found = false;

    public fn toJson() -> string {
        return "{\"id\":" + JsonUtil.quote(this.id)
            + ",\"user_id\":" + JsonUtil.quote(this.userId)
            + ",\"name\":" + JsonUtil.quote(this.name)
            + ",\"color\":" + JsonUtil.quote(this.color)
            + ",\"sort_order\":${this.sortOrder}"
            + ",\"todo_count\":${this.todoCount}"
            + ",\"created_at\":" + JsonUtil.quote(this.createdAt)
            + ",\"updated_at\":" + JsonUtil.quote(this.updatedAt)
            + "}";
    }
}
