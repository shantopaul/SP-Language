import std.io;
import std.math;
import std.time;

class User {
    string name;
    int age;

    User(string name, int age) {
        this.name = name;
        this.age = age;
    }
}

class UserService {
    User cachedUser;

    fn setUser(User u) {
        this.cachedUser = u;
    }

    fn getUser() -> User {
        return this.cachedUser;
    }
}

fn main() {
    println("=== Modules & Imports Showcase ===");

    UserService service = new UserService();
    User u = new User("Shanto Paul", 21);
    service.setUser(u);

    User retrieved = service.getUser();
    println("User: " + retrieved.name + ", age " + retrieved.age);

    // Using imported stdlib modules
    double root = Math.sqrt(144.0);
    println("std.math Math.sqrt(144): " + root);

    // Phase 4 slice D: Time.now() returns int64 epoch-ms.
    int64 nowMs = Time.now();
    println("std.time Time.now() ms:  " + nowMs);
}
