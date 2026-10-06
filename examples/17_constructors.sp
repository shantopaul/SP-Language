class Product {
    string name;
    double price;
    int quantity;
    string category;

    // Parameterized constructor
    Product(string name, double price, int quantity, string category) {
        this.name = name;
        this.price = price;
        this.quantity = quantity;
        this.category = category;
    }

    fn totalValue() -> double {
        return this.price * this.quantity;
    }

    fn display() {
        println("┌─────────────────────────────────┐");
        println("│ Product:  " + this.name);
        println("│ Price:    $" + this.price);
        println("│ Stock:    " + this.quantity + " units");
        println("│ Category: " + this.category);
        println("│ Value:    $" + this.totalValue());
        println("└─────────────────────────────────┘");
    }
}

class Book {
    string title;
    string author;
    double price;

    // Parameterized constructor
    Book(string title, string author, double price) {
        this.title = title;
        this.author = author;
        this.price = price;
    }

    fn display() {
        println("┌─────────────────────────────────┐");
        println("│ Book:   " + this.title);
        println("│ Author: " + this.author);
        println("│ Price:  $" + this.price);
        println("└─────────────────────────────────┘");
    }
}

class ServerConfig {
    string host;
    int port;

    // Default constructor (synthesized by compiler)
    fn display() {
        println("ServerConfig: " + this.host + ":" + this.port);
    }
}

fn main() {
    Product laptop = new Product("MacBook Pro", 2499.99, 10, "Electronics");
    laptop.display();
    println("");

    Book book = new Book("SP Language Guide", "Shanto Paul", 29.99);
    book.display();
    println("");

    ServerConfig config = new ServerConfig();
    config.host = "127.0.0.1";
    config.port = 8080;
    config.display();
}
