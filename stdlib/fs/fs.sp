module std.fs;

// Standard library filesystem module providing high-level file and directory primitives.

public enum IOError {
    NotFound,
    PermissionDenied,
    AlreadyExists,
    InvalidInput,
    UnexpectedEOF,
    Other,
}

extern "C" {
    fn sp_rt_fs_read_text(string path) -> string;
    fn sp_rt_fs_write_text(string path, string content) -> bool;
    fn sp_rt_fs_append_text(string path, string content) -> bool;
    fn sp_rt_fs_exists(string path) -> bool;
    fn sp_rt_fs_delete(string path) -> bool;
    fn sp_rt_fs_create_dir(string path) -> bool;
    fn sp_rt_fs_list_dir(string path) -> string[];
    fn sp_rt_fs_is_file(string path) -> bool;
    fn sp_rt_fs_is_dir(string path) -> bool;
}

public fn readText(string path) -> string {
    return sp_rt_fs_read_text(path);
}

public fn tryReadText(string path) -> Result<string, IOError> {
    if (!sp_rt_fs_exists(path)) {
        return Result.Err(IOError.NotFound);
    }
    string content = sp_rt_fs_read_text(path);
    return Result.Ok(content);
}

public fn writeText(string path, string content) -> bool {
    return sp_rt_fs_write_text(path, content);
}

public fn tryWriteText(string path, string content) -> Result<bool, IOError> {
    bool ok = sp_rt_fs_write_text(path, content);
    if (!ok) {
        return Result.Err(IOError.PermissionDenied);
    }
    return Result.Ok(true);
}

public fn appendText(string path, string content) -> bool {
    return sp_rt_fs_append_text(path, content);
}

public fn tryAppendText(string path, string content) -> Result<bool, IOError> {
    bool ok = sp_rt_fs_append_text(path, content);
    if (!ok) {
        return Result.Err(IOError.PermissionDenied);
    }
    return Result.Ok(true);
}

public fn exists(string path) -> bool {
    return sp_rt_fs_exists(path);
}

public fn deleteFile(string path) -> bool {
    return sp_rt_fs_delete(path);
}

public fn delete(string path) -> bool {
    return sp_rt_fs_delete(path);
}

public fn tryDelete(string path) -> Result<bool, IOError> {
    if (!sp_rt_fs_exists(path)) {
        return Result.Err(IOError.NotFound);
    }
    bool ok = sp_rt_fs_delete(path);
    if (!ok) {
        return Result.Err(IOError.PermissionDenied);
    }
    return Result.Ok(true);
}

public fn createDir(string path) -> bool {
    return sp_rt_fs_create_dir(path);
}

public fn tryCreateDir(string path) -> Result<bool, IOError> {
    bool ok = sp_rt_fs_create_dir(path);
    if (!ok) {
        return Result.Err(IOError.PermissionDenied);
    }
    return Result.Ok(true);
}

public fn listDir(string path) -> string[] {
    return sp_rt_fs_list_dir(path);
}

// Phase 38 slice E: listDir returns [] both for an empty directory
// and for an error (missing path, not-a-dir), so the try* variant
// pre-checks exists()/isDir() — the same pattern as tryReadText
// (and the same TOCTOU shape: a path deleted between the check and
// the read lists as Ok([])).
public fn tryListDir(string path) -> Result<string[], IOError> {
    if (!sp_rt_fs_exists(path)) {
        return Result.Err(IOError.NotFound);
    }
    if (!sp_rt_fs_is_dir(path)) {
        return Result.Err(IOError.InvalidInput);
    }
    return Result.Ok(sp_rt_fs_list_dir(path));
}

public fn isFile(string path) -> bool {
    return sp_rt_fs_is_file(path);
}

public fn isDir(string path) -> bool {
    return sp_rt_fs_is_dir(path);
}

public class FS {
    public static fn readText(string path) -> string {
        return sp_rt_fs_read_text(path);
    }

    public static fn tryReadText(string path) -> Result<string, IOError> {
        return tryReadText(path);
    }

    public static fn writeText(string path, string content) -> bool {
        return sp_rt_fs_write_text(path, content);
    }

    public static fn tryWriteText(string path, string content) -> Result<bool, IOError> {
        return tryWriteText(path, content);
    }

    public static fn appendText(string path, string content) -> bool {
        return sp_rt_fs_append_text(path, content);
    }

    public static fn tryAppendText(string path, string content) -> Result<bool, IOError> {
        return tryAppendText(path, content);
    }

    public static fn exists(string path) -> bool {
        return sp_rt_fs_exists(path);
    }

    public static fn delete(string path) -> bool {
        return sp_rt_fs_delete(path);
    }

    public static fn tryDelete(string path) -> Result<bool, IOError> {
        return tryDelete(path);
    }

    public static fn deleteFile(string path) -> bool {
        return sp_rt_fs_delete(path);
    }

    public static fn createDir(string path) -> bool {
        return sp_rt_fs_create_dir(path);
    }

    public static fn tryCreateDir(string path) -> Result<bool, IOError> {
        return tryCreateDir(path);
    }

    public static fn listDir(string path) -> string[] {
        return sp_rt_fs_list_dir(path);
    }

    public static fn tryListDir(string path) -> Result<string[], IOError> {
        return tryListDir(path);
    }

    public static fn isFile(string path) -> bool {
        return sp_rt_fs_is_file(path);
    }

    public static fn isDir(string path) -> bool {
        return sp_rt_fs_is_dir(path);
    }
}

public class File {
    public static fn readText(string path) -> string {
        return sp_rt_fs_read_text(path);
    }

    public static fn tryReadText(string path) -> Result<string, IOError> {
        return tryReadText(path);
    }

    public static fn writeText(string path, string content) -> bool {
        return sp_rt_fs_write_text(path, content);
    }

    public static fn tryWriteText(string path, string content) -> Result<bool, IOError> {
        return tryWriteText(path, content);
    }

    public static fn appendText(string path, string content) -> bool {
        return sp_rt_fs_append_text(path, content);
    }

    public static fn exists(string path) -> bool {
        return sp_rt_fs_exists(path);
    }

    public static fn delete(string path) -> bool {
        return sp_rt_fs_delete(path);
    }

    public static fn tryDelete(string path) -> Result<bool, IOError> {
        return tryDelete(path);
    }

    public static fn deleteFile(string path) -> bool {
        return sp_rt_fs_delete(path);
    }

    public static fn isFile(string path) -> bool {
        return sp_rt_fs_is_file(path);
    }
}
