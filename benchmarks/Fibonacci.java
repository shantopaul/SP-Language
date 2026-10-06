public class Fibonacci {
    public static long fib(int n) {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }

    public static void main(String[] args) {
        int n = 35;
        long res = fib(n);
        System.out.println("fib(35) = " + res);
    }
}
