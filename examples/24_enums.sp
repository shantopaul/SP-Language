enum Direction {
    North,
    South,
    East,
    West
}

enum HttpStatus {
    OK = 200,
    NotFound = 404,
    ServerError = 500
}

enum LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
}

fn describeDirection(Direction dir) -> string {
    string desc = "Unknown";
    match dir {
        Direction.North => desc = "Going North",
        Direction.South => desc = "Going South",
        Direction.East  => desc = "Going East",
        Direction.West  => desc = "Going West",
        _ => desc = "Unknown"
    }
    return desc;
}

fn logMessage(LogLevel level, string message) {
    match level {
        LogLevel.Debug   => println("[DEBUG]   " + message),
        LogLevel.Info    => println("[INFO]    " + message),
        LogLevel.Warning => println("[WARNING] " + message),
        LogLevel.Error   => println("[ERROR]   " + message),
        LogLevel.Fatal   => println("[FATAL]   " + message),
        _ => println("[OTHER]   " + message)
    }
}

fn main() {
    Direction dir = Direction.North;
    println(describeDirection(dir));

    HttpStatus status = HttpStatus.OK;
    println("HTTP " + status);

    println("");
    println("=== Log Messages ===");
    logMessage(LogLevel.Info, "Application started");
    logMessage(LogLevel.Debug, "Loading config...");
    logMessage(LogLevel.Warning, "Memory usage high");
    logMessage(LogLevel.Error, "Connection failed");
}
