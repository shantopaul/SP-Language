module models.user;

import util.json;

public class User {
    public string id = "";
    public string email = "";
    public string name = "";
    public string passwordHash = "";
    public string createdAt = "";
    public string updatedAt = "";
    public bool found = false;

    /// Serialize user fields without exposing passwordHash.
    public fn toPublicJson() -> string {
        return "{\"id\":" + JsonUtil.quote(this.id)
            + ",\"email\":" + JsonUtil.quote(this.email)
            + ",\"name\":" + JsonUtil.quote(this.name)
            + ",\"created_at\":" + JsonUtil.quote(this.createdAt)
            + ",\"updated_at\":" + JsonUtil.quote(this.updatedAt)
            + "}";
    }
}
