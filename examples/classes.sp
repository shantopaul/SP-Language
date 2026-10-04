import std.io;

class Person {
    protected string name;
    protected int age;

    Person(string name, int age) {
        this.name = name;
        this.age = age;
    }

    fn introduce() {
        println("Hi, I am ${name}, ${age} years old.");
    }
}

class Student extends Person {
    private string studentId;

    Student(string name, int age, string studentId) {
        super(name, age);
        this.studentId = studentId;
    }

    override fn introduce() {
        println("Student: ${name} [ID: ${studentId}], age ${age}.");
    }
}

fn main() {
    Person p = new Person("Shanto", 21);
    p.introduce();

    Student s = new Student("Alice", 20, "STU-1042");
    s.introduce();
}
