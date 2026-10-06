module std.io;

extern "C" {
    fn sp_rt_println_string(string str);
    fn sp_rt_println_int(int val);
    fn sp_rt_println_double(double val);
    fn sp_rt_println_bool(bool val);
    fn sp_rt_print_string(string str);
    fn sp_rt_print_int(int val);
    fn sp_rt_print_double(double val);
    fn sp_rt_print_bool(bool val);
    fn sp_rt_read_line() -> string;
    fn sp_rt_eprint_string(string str);
    fn sp_rt_eprintln_string(string str);
    fn sp_rt_flush();
    fn sp_rt_eflush();
}

public fn println() {
    sp_rt_println_string("");
}

public fn println(string message) {
    sp_rt_println_string(message);
}

public fn println(int value) {
    sp_rt_println_int(value);
}

public fn println(double value) {
    sp_rt_println_double(value);
}

public fn println(bool value) {
    sp_rt_println_bool(value);
}

public fn print(string message) {
    sp_rt_print_string(message);
}

public fn print(int value) {
    sp_rt_print_int(value);
}

public fn print(double value) {
    sp_rt_print_double(value);
}

public fn print(bool value) {
    sp_rt_print_bool(value);
}

public fn readLine() -> string {
    return sp_rt_read_line();
}

public fn readln() -> string {
    return sp_rt_read_line();
}

public fn eprint(string message) {
    sp_rt_eprint_string(message);
}

public fn eprintln(string message) {
    sp_rt_eprintln_string(message);
}

public fn flush() {
    sp_rt_flush();
}

public fn eflush() {
    sp_rt_eflush();
}

public class IO {
    public static fn println(string message) {
        sp_rt_println_string(message);
    }

    public static fn print(string message) {
        sp_rt_print_string(message);
    }

    public static fn readLine() -> string {
        return sp_rt_read_line();
    }

    public static fn eprintln(string message) {
        sp_rt_eprintln_string(message);
    }

    public static fn flush() {
        sp_rt_flush();
    }
}
