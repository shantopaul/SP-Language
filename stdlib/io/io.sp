module std.io;

extern "C" {
    fn sp_rt_println_string(string str);
    fn sp_rt_println_int(int val);
    fn sp_rt_println_double(double val);
    fn sp_rt_println_bool(bool val);
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
