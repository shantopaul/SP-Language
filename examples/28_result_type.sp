class Result<T> {
    bool isOk;
    T value;
    string errorMsg;

    fn isSuccess() -> bool {
        return this.isOk;
    }

    fn getValue() -> T {
        return this.value;
    }

    fn getError() -> string {
        return this.errorMsg;
    }
}

fn divide(int a, int b) -> Result<int> {
    Result<int> res = new Result<int>();
    if (b == 0) {
        res.isOk = false;
        res.errorMsg = "Cannot divide " + a + " by zero";
    } else {
        res.isOk = true;
        res.value = a / b;
    }
    return res;
}

fn parseNumber(string input) -> Result<int> {
    Result<int> res = new Result<int>();
    if (input == "42") {
        res.isOk = true;
        res.value = 42;
    } else if (input == "100") {
        res.isOk = true;
        res.value = 100;
    } else {
        res.isOk = false;
        res.errorMsg = "Invalid number: '" + input + "'";
    }
    return res;
}

fn printResult(Result<int> res) {
    if (res.isSuccess()) {
        println("Success: " + res.getValue());
    } else {
        println("Error: " + res.getError());
    }
}

fn main() {
    println("=== Result Type Error Handling ===");

    Result<int> r1 = divide(100, 5);
    printResult(r1);

    Result<int> r2 = divide(100, 0);
    printResult(r2);

    Result<int> p1 = parseNumber("42");
    printResult(p1);

    Result<int> p2 = parseNumber("hello");
    printResult(p2);
}
