module std.fs;

// Standard library filesystem module providing high-level file and directory primitives.

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

public fn writeText(string path, string content) -> bool {
    return sp_rt_fs_write_text(path, content);
}

public fn appendText(string path, string content) -> bool {
    return sp_rt_fs_append_text(path, content);
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

public fn createDir(string path) -> bool {
    return sp_rt_fs_create_dir(path);
}

public fn listDir(string path) -> string[] {
    return sp_rt_fs_list_dir(path);
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

    public static fn writeText(string path, string content) -> bool {
        return sp_rt_fs_write_text(path, content);
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

    public static fn deleteFile(string path) -> bool {
        return sp_rt_fs_delete(path);
    }

    public static fn createDir(string path) -> bool {
        return sp_rt_fs_create_dir(path);
    }

    public static fn listDir(string path) -> string[] {
        return sp_rt_fs_list_dir(path);
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

    public static fn writeText(string path, string content) -> bool {
        return sp_rt_fs_write_text(path, content);
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

    public static fn deleteFile(string path) -> bool {
        return sp_rt_fs_delete(path);
    }

    public static fn isFile(string path) -> bool {
        return sp_rt_fs_is_file(path);
    }
}
