fn classicSwitchDemo(int day) {
    println("--- Classic Switch ---");
    switch (day) {
        case 1:
            println("Monday");
            break;
        case 2:
            println("Tuesday");
            break;
        case 3:
            println("Wednesday");
            break;
        case 4:
            println("Thursday");
            break;
        case 5:
            println("Friday");
            break;
        case 6:
            println("Saturday 🎉");
            break;
        case 7:
            println("Sunday 🎉");
            break;
        default:
            println("Invalid day");
    }
}

fn modernMatchDemo() {
    int day = 3;

    // ═══════════════════════════════════════
    //  MATCH EXPRESSION (cleaner syntax)
    // ═══════════════════════════════════════

    println("--- Modern Match ---");
    match day {
        1 => println("Monday"),
        2 => println("Tuesday"),
        3 => println("Wednesday"),
        4 => println("Thursday"),
        5 => println("Friday"),
        6 => println("Saturday 🎉"),
        7 => println("Sunday 🎉"),
        _ => println("Invalid day")
    }

    // ═══════════════════════════════════════
    //  MATCH WITH STRINGS
    // ═══════════════════════════════════════

    string command = "start";

    match command {
        "start"   => println("Starting server..."),
        "stop"    => println("Stopping server..."),
        "restart" => println("Restarting server..."),
        "status"  => println("Server is running."),
        _         => println("Unknown command: ${command}")
    }

    // ═══════════════════════════════════════
    //  MATCH WITH ENUM
    // ═══════════════════════════════════════

    enum Season { Spring, Summer, Autumn, Winter }

    Season current = Season.Summer;

    match current {
        Season.Spring => println("🌸 Flowers blooming"),
        Season.Summer => println("☀️ Beach time!"),
        Season.Autumn => println("🍂 Leaves falling"),
        Season.Winter => println("❄️ Stay warm!")
    }
}

fn main() {
    classicSwitchDemo(3);
    modernMatchDemo();
}
