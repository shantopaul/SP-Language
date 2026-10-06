class Account {
    string owner;
    double balance;

    Account(string owner, double initial) {
        this.owner = owner;
        this.balance = initial;
    }

    fn deposit(double amount) {
        if (amount <= 0.0) {
            println("Invalid amount");
            return;
        }
        this.balance = this.balance + amount;
        println("Deposited $" + amount + " to " + this.owner);
    }

    fn withdraw(double amount) {
        if (amount <= 0.0 || amount > this.balance) {
            println("Invalid or insufficient funds");
            return;
        }
        this.balance = this.balance - amount;
        println("Withdrew $" + amount + " from " + this.owner);
    }

    fn transfer(Account target, double amount) {
        if (amount <= 0.0 || amount > this.balance) {
            println("Transfer failed");
            return;
        }
        this.balance = this.balance - amount;
        target.balance = target.balance + amount;
        println("Transferred $" + amount + " from " + this.owner + " to " + target.owner);
    }

    fn showBalance() {
        println(this.owner + "'s balance: $" + this.balance);
    }
}

class Task {
    string title;
    bool done;

    Task(string title) {
        this.title = title;
        this.done = false;
    }

    fn complete() {
        this.done = true;
    }

    fn display() {
        if (this.done) {
            println("  [x] " + this.title);
        } else {
            println("  [ ] " + this.title);
        }
    }
}

fn main() {
    println("===============================");
    println("  SP Complete Programs Showcase");
    println("===============================");
    println("");

    println("--- Program 1: Banking System ---");
    Account shanto = new Account("Shanto", 5000.0);
    Account alice = new Account("Alice", 3000.0);

    shanto.deposit(2000.0);
    shanto.withdraw(500.0);
    shanto.transfer(alice, 1000.0);

    println("");
    shanto.showBalance();
    alice.showBalance();

    println("");
    println("--- Program 2: Task Tracker ---");
    Task t1 = new Task("Learn SP syntax");
    Task t2 = new Task("Build native compiler");
    Task t3 = new Task("Run test suite");

    t1.complete();
    t2.complete();
    t3.complete();

    t1.display();
    t2.display();
    t3.display();

    println("");
    println("All complete programs executed successfully! 🚀");
}
