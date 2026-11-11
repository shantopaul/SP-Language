module util.logger;

import std.io;
import std.time;
import util.json;

public class AppLog {
    public static fn info(string component, string message) {
        let ts = Time.format(Time.now());
        println("{\"ts\":" + JsonUtil.quote(ts)
            + ",\"level\":\"INFO\""
            + ",\"component\":" + JsonUtil.quote(component)
            + ",\"message\":" + JsonUtil.quote(message) + "}");
    }

    public static fn warn(string component, string message) {
        let ts = Time.format(Time.now());
        println("{\"ts\":" + JsonUtil.quote(ts)
            + ",\"level\":\"WARN\""
            + ",\"component\":" + JsonUtil.quote(component)
            + ",\"message\":" + JsonUtil.quote(message) + "}");
    }

    public static fn error(string component, string message) {
        let ts = Time.format(Time.now());
        eprintln("{\"ts\":" + JsonUtil.quote(ts)
            + ",\"level\":\"ERROR\""
            + ",\"component\":" + JsonUtil.quote(component)
            + ",\"message\":" + JsonUtil.quote(message) + "}");
    }
}
