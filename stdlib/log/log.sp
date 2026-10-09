module std.log;

import std.io;

// ============================================================================
// Standard Library Structured Logging (std.log) (SP 2.9)
// Provides JSON-formatted structured logging with ISO 8601 timestamps,
// severity levels, contextual fields, and Console, File, and Memory appenders.
// ============================================================================

extern "C" {
    fn sp_rt_logger_create(string name, int level) -> ptr;
    fn sp_rt_logger_destroy(ptr logger);
    fn sp_rt_logger_default() -> ptr;
    fn sp_rt_logger_set_level(ptr logger, int level);
    fn sp_rt_logger_get_level(ptr logger) -> int;
    fn sp_rt_logger_add_console(ptr logger);
    fn sp_rt_logger_add_file(ptr logger, string filepath, int max_bytes, int max_backups);
    fn sp_rt_logger_add_memory(ptr logger, int capacity) -> ptr;
    fn sp_rt_logger_log(ptr logger, int level, string message, string fields_json);
    fn sp_rt_logger_with(ptr logger, string key, string val) -> ptr;
    fn sp_rt_memory_appender_get(ptr appender, int index) -> string;
    fn sp_rt_memory_appender_size(ptr appender) -> int;
    fn sp_rt_memory_appender_clear(ptr appender);
    fn sp_rt_memory_appender_destroy(ptr appender);
}

public class MemoryAppender {
    public ptr handle;

    public MemoryAppender(ptr h) {
        this.handle = h;
    }

    public fn size() -> int {
        if (this.handle == null) { return 0; }
        return sp_rt_memory_appender_size(this.handle);
    }

    public fn get(int index) -> string {
        if (this.handle == null) { return ""; }
        return sp_rt_memory_appender_get(this.handle, index);
    }

    public fn clear() {
        if (this.handle != null) {
            sp_rt_memory_appender_clear(this.handle);
        }
    }
}

public class Logger {
    public ptr handle;
    public string name;

    public static fn levelTrace() -> int { return 0; }
    public static fn levelDebug() -> int { return 1; }
    public static fn levelInfo() -> int { return 2; }
    public static fn levelWarn() -> int { return 3; }
    public static fn levelError() -> int { return 4; }
    public static fn levelFatal() -> int { return 5; }

    public Logger(string name = "app", int level = 2) {
        this.name = name;
        this.handle = sp_rt_logger_create(name, level);
        sp_rt_logger_add_console(this.handle);
    }

    public static fn create(string name = "app", int level = 2) -> Logger {
        let l = new Logger(name, level);
        return l;
    }

    public static fn getGlobal() -> Logger {
        let l = new Logger();
        if (l.handle != null) {
            sp_rt_logger_destroy(l.handle);
        }
        l.handle = sp_rt_logger_default();
        l.name = "default";
        return l;
    }

    public fn setLevel(int level) {
        if (this.handle != null) {
            sp_rt_logger_set_level(this.handle, level);
        }
    }

    public fn getLevel() -> int {
        if (this.handle == null) { return 2; }
        return sp_rt_logger_get_level(this.handle);
    }

    public fn addConsole() {
        if (this.handle != null) {
            sp_rt_logger_add_console(this.handle);
        }
    }

    public fn addFile(string filepath, int maxBytes = 10485760, int maxBackups = 5) {
        if (this.handle != null) {
            sp_rt_logger_add_file(this.handle, filepath, maxBytes, maxBackups);
        }
    }

    public fn addMemory(int capacity = 1000) -> MemoryAppender {
        if (this.handle == null) { return new MemoryAppender(null); }
        let h = sp_rt_logger_add_memory(this.handle, capacity);
        return new MemoryAppender(h);
    }

    public fn log(int level, string message) {
        if (this.handle != null) {
            sp_rt_logger_log(this.handle, level, message, "");
        }
    }

    public fn logWith(int level, string message, string fieldsJson) {
        if (this.handle != null) {
            sp_rt_logger_log(this.handle, level, message, fieldsJson);
        }
    }

    public fn trace(string message) {
        this.log(0, message);
    }

    public fn traceWith(string message, string fieldsJson) {
        this.logWith(0, message, fieldsJson);
    }

    public fn debug(string message) {
        this.log(1, message);
    }

    public fn debugWith(string message, string fieldsJson) {
        this.logWith(1, message, fieldsJson);
    }

    public fn info(string message) {
        this.log(2, message);
    }

    public fn infoWith(string message, string fieldsJson) {
        this.logWith(2, message, fieldsJson);
    }

    public fn warn(string message) {
        this.log(3, message);
    }

    public fn warnWith(string message, string fieldsJson) {
        this.logWith(3, message, fieldsJson);
    }

    public fn error(string message) {
        this.log(4, message);
    }

    public fn errorWith(string message, string fieldsJson) {
        this.logWith(4, message, fieldsJson);
    }

    public fn fatal(string message) {
        this.log(5, message);
    }

    public fn fatalWith(string message, string fieldsJson) {
        this.logWith(5, message, fieldsJson);
    }

    public fn withField(string key, string val) -> Logger {
        let child = new Logger();
        if (child.handle != null) {
            sp_rt_logger_destroy(child.handle);
        }
        child.handle = sp_rt_logger_with(this.handle, key, val);
        child.name = this.name;
        return child;
    }

    public fn field(string key, string val) -> Logger {
        return this.withField(key, val);
    }
}

public class Log {
    public static fn trace(string message) {
        Logger.getGlobal().trace(message);
    }

    public static fn traceWith(string message, string fieldsJson) {
        Logger.getGlobal().traceWith(message, fieldsJson);
    }

    public static fn debug(string message) {
        Logger.getGlobal().debug(message);
    }

    public static fn debugWith(string message, string fieldsJson) {
        Logger.getGlobal().debugWith(message, fieldsJson);
    }

    public static fn info(string message) {
        Logger.getGlobal().info(message);
    }

    public static fn infoWith(string message, string fieldsJson) {
        Logger.getGlobal().infoWith(message, fieldsJson);
    }

    public static fn warn(string message) {
        Logger.getGlobal().warn(message);
    }

    public static fn warnWith(string message, string fieldsJson) {
        Logger.getGlobal().warnWith(message, fieldsJson);
    }

    public static fn error(string message) {
        Logger.getGlobal().error(message);
    }

    public static fn errorWith(string message, string fieldsJson) {
        Logger.getGlobal().errorWith(message, fieldsJson);
    }

    public static fn fatal(string message) {
        Logger.getGlobal().fatal(message);
    }

    public static fn fatalWith(string message, string fieldsJson) {
        Logger.getGlobal().fatalWith(message, fieldsJson);
    }

    public static fn withField(string key, string val) -> Logger {
        return Logger.getGlobal().withField(key, val);
    }

    public static fn field(string key, string val) -> Logger {
        return Logger.getGlobal().withField(key, val);
    }
}
