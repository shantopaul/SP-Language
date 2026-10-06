interface Serializable {
    fn serialize() -> string;
}

interface Displayable {
    fn display();
}

class User implements Serializable, Displayable {
    string name;
    int age;

    User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    fn serialize() -> string {
        return "{\"name\":\"" + this.name + "\",\"age\":" + this.age + "}";
    }

    fn display() {
        println("User: " + this.name + " (" + this.age + ")");
    }
}

fn main() {
    println("=== Traits (Interface Composition) Demo ===");

    User user = new User("Shanto", 21);
    user.display();
    println("JSON: " + user.serialize());
}
