import std.time;

fn main() {
    println("--- std.time: Time and Duration ---");

    // Phase 4 slice D: epoch values are int64 (see stdlib/time/time.sp).
    int64 startMs = now();
    int64 startSecs = nowSeconds();
    println("Current epoch ms > 0: ${startMs > 0}");
    println("Current epoch secs > 0: ${startSecs > 0}");

    // Sleep for 25ms
    sleep(25);
    int64 endMs = now();
    int64 elapsed = endMs - startMs;
    println("Elapsed ms >= 20: ${elapsed >= 20}");

    // ISO 8601 Formatting
    string iso = formatTime(1700000000);
    println("Formatted 1700000000: ${iso}");

    // ISO 8601 Parsing
    int64 parsed = parseTime("2023-11-14T22:13:20Z");
    println("Parsed back: ${parsed}");

    // Duration class
    Duration d1 = Duration.fromSeconds(5);
    println("Duration 5s toMillis: ${d1.toMillis()}");
    println("Duration 5s toSeconds: ${d1.toSeconds()}");

    Duration d2 = Duration.fromMinutes(2);
    println("Duration 2m toMillis: ${d2.toMillis()}");

    // Static Time class
    // Phase 4 slice D: Time.now() returns int64 epoch-ms.
    int64 timeNow = Time.now();
    println("Time.now > 0: ${timeNow > 0}");

    println("std.time complete!");
}
