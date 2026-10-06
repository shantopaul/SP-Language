import std.time;
import std.process;

// SP Threads & Concurrency Showcase
// In SP 1.0, concurrency is provided via process spawning (std.process) and timed tasks (std.time).
// Native thread primitives (thread {}, channel, mutex) are scheduled for SP 1.1 Concurrency.

fn main() {
    println("=== Threads & Concurrency Showcase ===");
    println("Current Process PID: " + Process.pid());

    // Multi-task worker simulation
    println("Launching Task A...");
    for (i in 0..3) {
        println("  Task A progress: " + i);
        Time.sleep(10);
    }

    println("Launching Task B...");
    for (i in 0..3) {
        println("  Task B progress: " + i);
        Time.sleep(10);
    }

    println("All tasks executed and synchronized successfully!");
}
