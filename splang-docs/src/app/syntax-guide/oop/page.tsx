import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Classes, Structs & Interfaces",
  description: "Object-oriented programming, vtables, inheritance, and value structs in SP.",
};

export default function OopPage() {
  const code = `import std.io;

// Interface contract
interface Greeter {
    fn greet() -> void;
}

// Base class with virtual method
class Person : Greeter {
    protected string name;
    
    public Person(string name) {
        this.name = name;
    }
    
    public virtual fn greet() -> void {
        println("Hello, I am \${this.name}");
    }
}

// Single inheritance & method override
class Employee : Person {
    private string role;
    
    public Employee(string name, string role) : super(name) {
        this.role = role;
    }
    
    public override fn greet() -> void {
        println("Greetings, I am \${this.name}, working as \${this.role}");
    }
}

// Lightweight value-type struct (stack allocated)
struct Point {
    int x;
    int y;
}

fn main() -> void {
    Person p = new Employee("Sarah", "Systems Engineer");
    p.greet();
    
    Point pt = Point(10, 20);
    println("Point: (\${pt.x}, \${pt.y})");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "OOP, Classes & Structs" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Object-Oriented Programming</h1>
      <p className="text-sm text-slate-300">
        SP supports full Object-Oriented Programming including reference classes with virtual tables, single inheritance, interface implementation contracts, and stack-allocated value structs.
      </p>

      <CodeBlock code={code} filename="oop.sp" />

      <h2 className="text-xl font-bold text-white pt-2">Classes vs Structs</h2>
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 text-xs sm:text-sm">
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h3 className="font-bold text-cyan-400 mb-2">Class (Reference Type)</h3>
          <p className="text-slate-400">
            Allocated on heap with <code>new</code>. Supports single inheritance, vtable virtual dispatch, polymorphism, and access modifiers.
          </p>
        </div>
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h3 className="font-bold text-emerald-400 mb-2">Struct (Value Type)</h3>
          <p className="text-slate-400">
            Stack-allocated with zero heap allocator overhead. Passed by value and ideal for coordinate pairs, vector math, and compact records.
          </p>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Functions & Closures", href: "/syntax-guide/functions" }}
        next={{ title: "Zero-Cost Generics", href: "/syntax-guide/generics" }}
      />
    </div>
  );
}
