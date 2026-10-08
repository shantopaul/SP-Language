module std.json;

// ============================================================================
// Standard Library High-Performance JSON Parser & Serializer (SP 1.6)
// ============================================================================

extern "C" {
    fn sp_rt_json_parse(string jsonStr) -> ptr;
    fn sp_rt_json_stringify(ptr handle, int indent) -> string;
    fn sp_rt_json_get_type(ptr handle) -> int;
    fn sp_rt_json_get_bool(ptr handle) -> bool;
    fn sp_rt_json_get_int(ptr handle) -> int64;
    fn sp_rt_json_get_double(ptr handle) -> double;
    fn sp_rt_json_get_string(ptr handle) -> string;
    fn sp_rt_json_arr_size(ptr handle) -> int;
    fn sp_rt_json_arr_get(ptr handle, int idx) -> ptr;
    fn sp_rt_json_arr_add(ptr handle, ptr itemHandle);
    fn sp_rt_json_obj_get(ptr handle, string key) -> ptr;
    fn sp_rt_json_obj_set(ptr handle, string key, ptr itemHandle);
    fn sp_rt_json_obj_has(ptr handle, string key) -> bool;

    fn sp_rt_json_new_null() -> ptr;
    fn sp_rt_json_new_bool(bool b) -> ptr;
    fn sp_rt_json_new_int(int64 i) -> ptr;
    fn sp_rt_json_new_double(double d) -> ptr;
    fn sp_rt_json_new_string(string s) -> ptr;
    fn sp_rt_json_new_array() -> ptr;
    fn sp_rt_json_new_object() -> ptr;
    fn sp_rt_json_destroy(ptr handle);

    // Typed Data Serialization (SP 2.4)
    fn sp_rt_json_register_class(string className, int objectSize, bool hasVtable);
    fn sp_rt_json_register_field(string className, string fieldName, string jsonKey, int typeKind, string typeName, int byteOffset, bool isIgnored, bool isNullable);
    fn sp_rt_json_decode_typed(string className, string jsonStr, ptr outErr) -> ptr;
    fn sp_rt_json_encode_typed(string className, ptr obj, int indent) -> string;
    fn sp_rt_json_problem_details(string title, int status, string detail, string instance, string errorsJson) -> string;
}

public enum JsonType {
    Null,
    Bool,
    Int,
    Double,
    String,
    Array,
    Object,
    Error,
}

public class JsonValue {
    public ptr handle;

    public static fn parse(string jsonStr) -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_parse(jsonStr);
        return v;
    }

    public static fn nullVal() -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_new_null();
        return v;
    }

    public static fn boolVal(bool b) -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_new_bool(b);
        return v;
    }

    public static fn intVal(int64 i) -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_new_int(i);
        return v;
    }

    public static fn doubleVal(double d) -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_new_double(d);
        return v;
    }

    public static fn stringVal(string s) -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_new_string(s);
        return v;
    }

    public static fn arrayVal() -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_new_array();
        return v;
    }

    public static fn objectVal() -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_new_object();
        return v;
    }

    public fn isValid() -> bool {
        return this.handle != null;
    }

    public fn stringify(int indent) -> string {
        return sp_rt_json_stringify(this.handle, indent);
    }

    public fn toString() -> string {
        return sp_rt_json_stringify(this.handle, 0);
    }

    public fn asBool() -> bool {
        return sp_rt_json_get_bool(this.handle);
    }

    public fn asInt() -> int64 {
        return sp_rt_json_get_int(this.handle);
    }

    public fn asDouble() -> double {
        return sp_rt_json_get_double(this.handle);
    }

    public fn asString() -> string {
        return sp_rt_json_get_string(this.handle);
    }

    public fn get(string key) -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_obj_get(this.handle, key);
        return v;
    }

    public fn set(string key, JsonValue item) {
        sp_rt_json_obj_set(this.handle, key, item.handle);
    }

    public fn has(string key) -> bool {
        return sp_rt_json_obj_has(this.handle, key);
    }

    public fn at(int idx) -> JsonValue {
        let v = new JsonValue();
        v.handle = sp_rt_json_arr_get(this.handle, idx);
        return v;
    }

    public fn add(JsonValue item) {
        sp_rt_json_arr_add(this.handle, item.handle);
    }

    public fn size() -> int {
        return sp_rt_json_arr_size(this.handle);
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_json_destroy(this.handle);
            this.handle = null;
        }
    }
}

public class ProblemDetails {
    public string type;
    public string title;
    public int status;
    public string detail;
    public string instance;
    public string errorsJson;

    public fn toJson() -> string {
        return sp_rt_json_problem_details(this.title, this.status, this.detail, this.instance, this.errorsJson);
    }
}

public class Json {
    public static fn parse(string jsonStr) -> JsonValue {
        return JsonValue.parse(jsonStr);
    }

    public static fn stringify(JsonValue val, int indent) -> string {
        return val.stringify(indent);
    }

    public static fn object() -> JsonValue {
        return JsonValue.objectVal();
    }

    public static fn array() -> JsonValue {
        return JsonValue.arrayVal();
    }

    // Typed Data Serialization (SP 2.4)
    public static fn registerClass(string className, int objectSize, bool hasVtable) {
        sp_rt_json_register_class(className, objectSize, hasVtable);
    }

    public static fn registerField(string className, string fieldName, string jsonKey, int typeKind, string typeName, int byteOffset, bool isIgnored, bool isNullable) {
        sp_rt_json_register_field(className, fieldName, jsonKey, typeKind, typeName, byteOffset, isIgnored, isNullable);
    }

    public static fn decodeTyped(string className, string jsonStr) -> ptr {
        return sp_rt_json_decode_typed(className, jsonStr, null);
    }

    public static fn encodeTyped(string className, ptr obj, int indent) -> string {
        return sp_rt_json_encode_typed(className, obj, indent);
    }

    public static fn problemDetails(string title, int status, string detail, string instance, string errorsJson) -> string {
        return sp_rt_json_problem_details(title, status, detail, instance, errorsJson);
    }
}

