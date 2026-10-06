interface Drawable {
    fn draw();
    fn getColor() -> string;
}

interface Resizable {
    fn resize(double factor);
    fn getArea() -> double;
}

class Circle implements Drawable, Resizable {
    double radius;
    string color;

    Circle(double radius, string color) {
        this.radius = radius;
        this.color = color;
    }

    fn draw() {
        println("Drawing Circle (r=" + this.radius + ", color=" + this.color + ")");
    }

    fn getColor() -> string {
        return this.color;
    }

    fn resize(double factor) {
        this.radius = this.radius * factor;
        println("Resized circle to radius " + this.radius);
    }

    fn getArea() -> double {
        return 3.14159 * this.radius * this.radius;
    }
}

class Rectangle implements Drawable, Resizable {
    double width;
    double height;
    string color;

    Rectangle(double w, double h, string color) {
        this.width = w;
        this.height = h;
        this.color = color;
    }

    fn draw() {
        println("Drawing Rectangle (" + this.width + "x" + this.height + ", color=" + this.color + ")");
    }

    fn getColor() -> string {
        return this.color;
    }

    fn resize(double factor) {
        this.width = this.width * factor;
        this.height = this.height * factor;
    }

    fn getArea() -> double {
        return this.width * this.height;
    }
}

fn main() {
    Circle c = new Circle(10.0, "Green");
    c.draw();
    println("Color: " + c.getColor());
    println("Circle area: " + c.getArea());
    c.resize(2.0);
    println("New circle area: " + c.getArea());

    println("");

    Rectangle r = new Rectangle(5.0, 8.0, "Yellow");
    r.draw();
    println("Rectangle area: " + r.getArea());
    r.resize(1.5);
    println("Resized rectangle area: " + r.getArea());
}
