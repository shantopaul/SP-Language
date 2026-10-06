class User {
    string name;
    int age;
    string email;

    // Constructor
    User(string name, int age, string email) {
        this.name = name;
        this.age = age;
        this.email = email;
    }

    // Methods
    fn introduce() {
        println("Hi! I'm ${name}, ${age} years old.");
        println("Email: ${email}");
    }

    fn isAdult() -> bool {
        return age >= 18;
    }

    fn birthday() {
        this.age = this.age + 1;
        println("🎂 Happy Birthday, ${name}! Now ${age}.");
    }
}

fn main() {
    User user1 = new User("Shanto", 21, "shanto@email.com");
    User user2 = new User("Alice", 17, "alice@email.com");

    user1.introduce();
    println("Is adult: ${user1.isAdult()}");
    println("");

    user2.introduce();
    println("Is adult: ${user2.isAdult()}");
    println("");

    user2.birthday();
    println("Is adult now: ${user2.isAdult()}");
}
