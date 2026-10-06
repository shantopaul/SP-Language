class BankAccount {
    public string ownerName;
    private double balance;
    private string accountNumber;
    protected string accountType;
    internal string branchCode;

    public BankAccount(string owner, double initial) {
        this.ownerName = owner;
        this.balance = initial;
        this.accountNumber = "ACC-123456789";
        this.accountType = "Savings";
        this.branchCode = "BR-001";
    }

    public fn deposit(double amount) {
        if (amount > 0.0) {
            this.balance = this.balance + amount;
            println("Deposited $" + amount + ". New balance: $" + this.balance);
        }
    }

    public fn withdraw(double amount) {
        if (amount > 0.0 && amount <= this.balance) {
            this.balance = this.balance - amount;
            println("Withdrew $" + amount + ". New balance: $" + this.balance);
        } else {
            println("Insufficient funds.");
        }
    }

    public fn getBalance() -> double {
        return this.balance;
    }

    private fn generateId() -> string {
        return "123456789";
    }

    public fn printAccountInfo() {
        println("Generated internal ID: " + this.generateId());
    }
}

fn main() {
    BankAccount account = new BankAccount("Shanto", 1000.0);

    println("Owner: " + account.ownerName);
    account.deposit(500.0);
    account.withdraw(200.0);
    println("Balance: $" + account.getBalance());
    account.printAccountInfo();
}
