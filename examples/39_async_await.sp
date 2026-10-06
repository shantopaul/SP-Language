import std.time;

// SP Async & Await Pattern Simulation
// In SP 1.0, non-blocking workflows are handled via Task/Future patterns with std.time.
// Native 'async' and 'await' syntax is part of the SP 1.1 Concurrency roadmap.

class TaskResult {
    bool isCompleted;
    string data;

    TaskResult(string d) {
        this.isCompleted = true;
        this.data = d;
    }
}

class AsyncTask {
    static fn fetchUserData(int id) -> TaskResult {
        println("Fetching user " + id + " asynchronously...");
        Time.sleep(50);
        return new TaskResult("User data for ID " + id);
    }
}

fn main() {
    println("=== Async & Concurrency Demo ===");
    println("Fetching data...");

    TaskResult u1 = AsyncTask.fetchUserData(1);
    println("Received: " + u1.data);

    TaskResult u2 = AsyncTask.fetchUserData(2);
    println("Received: " + u2.data);

    println("All asynchronous tasks finished!");
}
