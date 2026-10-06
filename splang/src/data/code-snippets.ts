export interface CodeSnippet {
  id: string;
  title: string;
  subtitle: string;
  code: string;
  output: string;
}

export const CODE_SNIPPETS: CodeSnippet[] = [
  {
    id: "basics",
    title: "Modern Basics",
    subtitle: "Type inference, clean string interpolation, and concise functions.",
    code: `import std.io;

fn greet(string name, int times = 1) -> void {
    for (int i = 0; i < times; i = i + 1) {
        println("Hello, \${name}! [Visit #\${i + 1}]");
    }
}

fn main() -> void {
    let language = "SP";
    let speed = 99.8;
    println("Welcome to \${language}! Execution efficiency: \${speed}%");
    greet(language, 2);
}`,
    output: `Welcome to SP! Execution efficiency: 99.8%
Hello, SP! [Visit #1]
Hello, SP! [Visit #2]`,
  },
  {
    id: "matching",
    title: "Pattern Matching",
    subtitle: "Exhaustive, expressive match constructs with value bindings.",
    code: `import std.io;

enum Status {
    Pending,
    Active,
    Suspended,
    Completed
}

fn describe(Status s) -> string {
    return match (s) {
        Status.Pending => "Task queued in worker pool",
        Status.Active => "Processing batch requests",
        Status.Suspended => "Execution halted on signal",
        Status.Completed => "All stages finished cleanly"
    };
}

fn main() -> void {
    let current = Status.Active;
    println("Status report: \${describe(current)}");
}`,
    output: `Status report: Processing batch requests`,
  },
  {
    id: "oop",
    title: "OOP & Interfaces",
    subtitle: "Virtual dispatch, constructors, and flexible single inheritance.",
    code: `import std.io;

interface Renderable {
    fn render() -> void;
}

class Shape : Renderable {
    protected string name;
    
    public Shape(string name) {
        this.name = name;
    }
    
    public virtual fn area() -> double {
        return 0.0;
    }
    
    public fn render() -> void {
        println("[\${this.name}] Area: \${this.area()}");
    }
}

class Circle : Shape {
    private double radius;
    
    public Circle(string name, double radius) : super(name) {
        this.radius = radius;
    }
    
    public override fn area() -> double {
        return 3.14159265 * this.radius * this.radius;
    }
}

fn main() -> void {
    Shape s = new Circle("MainCircle", 5.0);
    s.render();
}`,
    output: `[MainCircle] Area: 78.539816`,
  },
  {
    id: "generics",
    title: "Generics & Lambdas",
    subtitle: "Zero-cost monomorphized generics and first-class closures.",
    code: `import std.io;
import std.collections;

fn main() -> void {
    let numbers = [1, 2, 3, 4, 5, 6];
    
    // Higher-order functional pipelines
    let evens = numbers.filter((int n) => n % 2 == 0);
    let doubled = evens.map((int n) => n * 10);
    
    println("Original count: \${numbers.length()}");
    println("Doubled evens:");
    for (int item in doubled) {
        println(" -> \${item}");
    }
}`,
    output: `Original count: 6
Doubled evens:
 -> 20
 -> 40
 -> 60`,
  },
  {
    id: "stdlib",
    title: "Standard Library",
    subtitle: "Production-ready IO, Filesystem, Math, and OS modules out-of-the-box.",
    code: `import std.io;
import std.fs;
import std.time;
import std.math;

fn main() -> void {
    let filename = "config.sp.tmp";
    fs.writeText(filename, "mode=production\\nthreads=8");
    
    if (fs.exists(filename)) {
        let content = fs.readText(filename);
        println("Created: \${filename}");
        println("Content:\\n\${content}");
        fs.delete(filename);
    }
    
    let root = math.sqrt(256.0);
    println("Calculated root: \${root}");
}`,
    output: `Created: config.sp.tmp
Content:
mode=production
threads=8
Calculated root: 16`,
  },
];
