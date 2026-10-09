module std.crypto.password;

// ============================================================================
// Standard Library Password Hashing (Argon2id & Bcrypt) (SP 2.6)
// ============================================================================

extern "C" {
    fn sp_rt_password_hash(string password, string algorithm, int cost_or_memory, int iterations, int parallelism) -> string;
    fn sp_rt_password_verify(string password, string hash) -> bool;
}

public class PasswordOptions {
    public string algorithm;
    public int memoryKiB;
    public int iterations;
    public int parallelism;
    public int bcryptCost;

    public fn init() {
        this.algorithm = "argon2id";
        this.memoryKiB = 65536;
        this.iterations = 3;
        this.parallelism = 1;
        this.bcryptCost = 10;
    }

    public static fn argon2id(int memoryKiB, int iterations, int parallelism) -> PasswordOptions {
        var opts = new PasswordOptions();
        opts.init();
        opts.algorithm = "argon2id";
        opts.memoryKiB = memoryKiB;
        opts.iterations = iterations;
        opts.parallelism = parallelism;
        return opts;
    }

    public static fn bcrypt(int cost) -> PasswordOptions {
        var opts = new PasswordOptions();
        opts.init();
        opts.algorithm = "bcrypt";
        opts.bcryptCost = cost;
        return opts;
    }
}

public class Password {
    /// Hash a password using default production settings (Argon2id, RFC 9106)
    public static fn hash(string plainText) -> string {
        return sp_rt_password_hash(plainText, "argon2id", 65536, 3, 1);
    }

    /// Hash a password with custom options (Argon2id or Bcrypt)
    public static fn hashWithOptions(string plainText, PasswordOptions options) -> string {
        if (options.algorithm == "bcrypt") {
            return sp_rt_password_hash(plainText, "bcrypt", options.bcryptCost, 0, 0);
        }
        return sp_rt_password_hash(plainText, "argon2id", options.memoryKiB, options.iterations, options.parallelism);
    }

    /// Verify a plainText password against a stored PHC / MCF hash
    public static fn verify(string plainText, string hash) -> bool {
        return sp_rt_password_verify(plainText, hash);
    }

    /// Hash using Argon2id with explicit memoryKiB, iterations, and parallelism
    public static fn argon2id(string plainText, int memoryKiB, int iterations, int parallelism) -> string {
        return sp_rt_password_hash(plainText, "argon2id", memoryKiB, iterations, parallelism);
    }

    /// Hash using Bcrypt with explicit cost factor (4-31)
    public static fn bcrypt(string plainText, int cost) -> string {
        return sp_rt_password_hash(plainText, "bcrypt", cost, 0, 0);
    }
}
