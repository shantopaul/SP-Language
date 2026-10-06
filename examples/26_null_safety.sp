class User {
    string name;
    string email;

    User(string name, string email) {
        this.name = name;
        this.email = email;
    }
}

fn findUser(int id) -> User? {
    if (id == 1) {
        return new User("Shanto", "shanto@email.com");
    }
    return null;
}

fn main() {
    println("=== Null Safety Demo ===");

    User? user1 = findUser(1);
    User? user2 = findUser(999);

    if (user1 != null) {
        println("User 1: " + user1?.name + " (" + user1?.email + ")");
    } else {
        println("User 1 not found");
    }

    if (user2 == null) {
        println("User 2 not found (null check passed)");
    } else {
        println("User 2: " + user2?.name);
    }
}
