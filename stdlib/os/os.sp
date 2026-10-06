module std.os;

// Standard library OS module providing environment variables, platform details, arguments, and process exit.

extern "C" {
    fn sp_rt_os_get_env(string name) -> string;
    fn sp_rt_os_set_env(string name, string val) -> bool;
    fn sp_rt_os_platform() -> string;
    fn sp_rt_os_arch() -> string;
    fn sp_rt_os_args() -> string[];
    fn sp_rt_os_exit(int code) -> void;
}

public fn getEnv(string name) -> string {
    return sp_rt_os_get_env(name);
}

public fn setEnv(string name, string val) -> bool {
    return sp_rt_os_set_env(name, val);
}

public fn platform() -> string {
    return sp_rt_os_platform();
}

public fn arch() -> string {
    return sp_rt_os_arch();
}

public fn args() -> string[] {
    return sp_rt_os_args();
}

public fn exit(int code) -> void {
    sp_rt_os_exit(code);
}

public class OS {
    public static fn getEnv(string name) -> string {
        return sp_rt_os_get_env(name);
    }

    public static fn setEnv(string name, string val) -> bool {
        return sp_rt_os_set_env(name, val);
    }

    public static fn platform() -> string {
        return sp_rt_os_platform();
    }

    public static fn arch() -> string {
        return sp_rt_os_arch();
    }

    public static fn args() -> string[] {
        return sp_rt_os_args();
    }

    public static fn exit(int code) -> void {
        sp_rt_os_exit(code);
    }
}
