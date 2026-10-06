module std.time;

// Standard library time module providing time tracking, duration, and sleeping primitives.

extern "C" {
    fn sp_rt_time_now_ms() -> int;
    fn sp_rt_time_now_secs() -> int;
    fn sp_rt_time_now_ns() -> int;
    fn sp_rt_time_sleep_ms(int ms) -> void;
    fn sp_rt_time_sleep_secs(int s) -> void;
    fn sp_rt_time_format_iso(int epoch_ms) -> string;
    fn sp_rt_time_parse_iso(string iso_str) -> int;
}

public fn now() -> int {
    return sp_rt_time_now_ms();
}

public fn nowSeconds() -> int {
    return sp_rt_time_now_secs();
}

public fn nowNanos() -> int {
    return sp_rt_time_now_ns();
}

public fn sleep(int ms) -> void {
    sp_rt_time_sleep_ms(ms);
}

public fn sleepSeconds(int s) -> void {
    sp_rt_time_sleep_secs(s);
}

public fn formatTime(int epoch_ms) -> string {
    return sp_rt_time_format_iso(epoch_ms);
}

public fn parseTime(string iso_str) -> int {
    return sp_rt_time_parse_iso(iso_str);
}

public class Duration {
    public int millis;

    public static fn fromMillis(int ms) -> Duration {
        Duration d = new Duration();
        d.millis = ms;
        return d;
    }

    public static fn fromSeconds(int s) -> Duration {
        Duration d = new Duration();
        d.millis = s * 1000;
        return d;
    }

    public static fn fromMinutes(int m) -> Duration {
        Duration d = new Duration();
        d.millis = m * 60000;
        return d;
    }

    public static fn fromHours(int h) -> Duration {
        Duration d = new Duration();
        d.millis = h * 3600000;
        return d;
    }

    public fn toMillis() -> int {
        return this.millis;
    }

    public fn toSeconds() -> int {
        return this.millis / 1000;
    }
}

public class Time {
    public static fn now() -> int {
        return sp_rt_time_now_ms();
    }

    public static fn nowSeconds() -> int {
        return sp_rt_time_now_secs();
    }

    public static fn nowNanos() -> int {
        return sp_rt_time_now_ns();
    }

    public static fn sleep(int ms) -> void {
        sp_rt_time_sleep_ms(ms);
    }

    public static fn sleepSeconds(int s) -> void {
        sp_rt_time_sleep_secs(s);
    }

    public static fn format(int epoch_ms) -> string {
        return sp_rt_time_format_iso(epoch_ms);
    }

    public static fn parse(string iso_str) -> int {
        return sp_rt_time_parse_iso(iso_str);
    }
}
