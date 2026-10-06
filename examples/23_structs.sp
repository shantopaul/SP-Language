import std.math;

struct Point {
    int x;
    int y;
}

struct Color {
    int r;
    int g;
    int b;
}

fn main() {
    Point p1;
    p1.x = 0;
    p1.y = 0;

    Point p2;
    p2.x = 3;
    p2.y = 4;

    println("Point 1: (" + p1.x + ", " + p1.y + ")");
    println("Point 2: (" + p2.x + ", " + p2.y + ")");

    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    double dist = Math.sqrt(dx * dx + dy * dy);
    println("Distance: " + dist);

    Color red;
    red.r = 255;
    red.g = 0;
    red.b = 0;
    println("Color: rgb(" + red.r + ", " + red.g + ", " + red.b + ")");
}
