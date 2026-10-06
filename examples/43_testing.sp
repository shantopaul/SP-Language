fn add(int a, int b) -> int {
    return a + b;
}

fn factorial(int n) -> int {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

fn isEven(int n) -> bool {
    return n % 2 == 0;
}

test "addition works" {
    assert(add(2, 3) == 5);
    assert(add(-1, 1) == 0);
    assert(add(0, 0) == 0);
}

test "factorial is correct" {
    assert(factorial(0) == 1);
    assert(factorial(1) == 1);
    assert(factorial(5) == 120);
}

test "even/odd detection" {
    assert(isEven(0) == true);
    assert(isEven(2) == true);
    assert(isEven(3) == false);
}

test "string operations" {
    string name = "Shanto";
    assert(name.length() == 6);
    assert(name.upper() == "SHANTO");
    assert(name.contains("han") == true);
}
