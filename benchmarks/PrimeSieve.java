public class PrimeSieve {
    public static boolean isPrime(int n) {
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;

        int i = 5;
        while (i * i <= n) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
            i += 6;
        }
        return true;
    }

    public static void main(String[] args) {
        int limit = 100000;
        int count = 0;

        for (int num = 2; num <= limit; num++) {
            if (isPrime(num)) {
                count++;
            }
        }

        System.out.println("Primes up to " + limit + ": " + count);
    }
}
