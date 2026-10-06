module std.process;

// Standard library process module providing command execution, background spawn, and process management.

extern "C" {
    fn sp_rt_process_exec(string cmd) -> int;
    fn sp_rt_process_spawn(string cmd) -> int;
    fn sp_rt_process_run(string cmd) -> string;
    fn sp_rt_process_pid() -> int;
}

public fn exec(string cmd) -> int {
    return sp_rt_process_exec(cmd);
}

public fn spawn(string cmd) -> int {
    return sp_rt_process_spawn(cmd);
}

public fn runCommand(string cmd) -> string {
    return sp_rt_process_run(cmd);
}

public fn pid() -> int {
    return sp_rt_process_pid();
}

public class Process {
    public static fn exec(string cmd) -> int {
        return sp_rt_process_exec(cmd);
    }

    public static fn spawn(string cmd) -> int {
        return sp_rt_process_spawn(cmd);
    }

    public static fn run(string cmd) -> string {
        return sp_rt_process_run(cmd);
    }

    public static fn pid() -> int {
        return sp_rt_process_pid();
    }
}
