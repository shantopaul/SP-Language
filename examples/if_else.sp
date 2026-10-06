fn main() {
    int age = 21;
    int score = 85;
    string role = "admin";

    // ═══════════════════════════════════════
    //  SIMPLE IF
    // ═══════════════════════════════════════

    if (age >= 18) {
        println("You are an adult.");
    }

    // ═══════════════════════════════════════
    //  IF / ELSE
    // ═══════════════════════════════════════

    if (age >= 18) {
        println("Welcome! You can enter.");
    } else {
        println("Sorry, you must be 18+.");
    }

    // ═══════════════════════════════════════
    //  IF / ELSE-IF / ELSE (Grade System)
    // ═══════════════════════════════════════

    if (score >= 90) {
        println("Grade: A  ⭐ Excellent!");
    } else if (score >= 80) {
        println("Grade: B  👍 Good job!");
    } else if (score >= 70) {
        println("Grade: C  📝 Average");
    } else if (score >= 60) {
        println("Grade: D  ⚠️ Below average");
    } else {
        println("Grade: F  ❌ Failed");
    }

    // ═══════════════════════════════════════
    //  NESTED IF
    // ═══════════════════════════════════════

    if (age >= 18) {
        if (role == "admin") {
            println("Admin access granted.");
        } else {
            println("User access granted.");
        }
    } else {
        println("Access denied.");
    }

    // ═══════════════════════════════════════
    //  LOGICAL CONDITIONS
    // ═══════════════════════════════════════

    bool hasLicense = true;

    if (age >= 18 && hasLicense) {
        println("You can drive! 🚗");
    }

    if (role == "admin" || role == "moderator") {
        println("You have elevated permissions.");
    }

    if (!hasLicense) {
        println("Get a license first.");
    }

    // ═══════════════════════════════════════
    //  TERNARY OPERATOR
    // ═══════════════════════════════════════

    string status = age >= 18 ? "Adult" : "Minor";
    println("Status: ${status}");
}
