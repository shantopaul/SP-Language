import std.os;

fn main() {
    println("--- Standard Library OS Demo ---");

    string plat = platform();
    println("Platform: ${plat}");

    string architecture = arch();
    println("Architecture: ${architecture}");

    string varKey = "SP_TEST_ENV";
    string varVal = "AntigravityOS";
    bool setSuccess = setEnv(varKey, varVal);
    println("SetEnv success: ${setSuccess}");

    string retrieved = getEnv(varKey);
    println("Retrieved env: ${retrieved}");

    string[] cliArgs = args();
    println("Args count: ${cliArgs.length()}");

    // Static OS class demo
    println("OS.platform(): ${OS.platform()}");
    println("OS.arch(): ${OS.arch()}");
    println("OS.getEnv(): ${OS.getEnv(varKey)}");

    println("All OS tests passed!");
}
