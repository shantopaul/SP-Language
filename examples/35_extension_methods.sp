class StringExt {
    static fn isEmail(string s) -> bool {
        return s.contains("@") && s.contains(".");
    }

    static fn reverse(string s) -> string {
        return s.reverse();
    }

    static fn wordCount(string s) -> int {
        return s.trim().split(" ").length();
    }
}

class IntExt {
    static fn isEven(int n) -> bool {
        return n % 2 == 0;
    }

    static fn isOdd(int n) -> bool {
        return n % 2 != 0;
    }

    static fn factorial(int n) -> int {
        int result = 1;
        for (int i = 2; i <= n; i++) {
            result = result * i;
        }
        return result;
    }
}

fn main() {
    println("=== Extension Methods Demo ===");

    // String extensions
    println("isEmail('test@email.com'): " + StringExt.isEmail("test@email.com"));
    println("isEmail('not-an-email'):   " + StringExt.isEmail("not-an-email"));
    println("reverse('Hello'):          " + StringExt.reverse("Hello"));
    println("wordCount('Hello SP World'): " + StringExt.wordCount("Hello SP World"));

    // Int extensions
    println("isEven(42):     " + IntExt.isEven(42));
    println("isOdd(7):       " + IntExt.isOdd(7));
    println("factorial(5):   " + IntExt.factorial(5));
}
