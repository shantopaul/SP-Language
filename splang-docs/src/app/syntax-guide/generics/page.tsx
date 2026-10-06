import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Zero-Cost Generics",
  description: "Generic classes, generic functions, and compile-time monomorphization in SP.",
};

export default function GenericsPage() {
  const code = `import std.io;

// Generic container class
class Box<T> {
    private T item;
    
    public Box(T item) {
        this.item = item;
    }
    
    public fn get() -> T {
        return this.item;
    }
}

// Generic standalone function
fn identity<T>(T val) -> T {
    return val;
}

fn main() -> void {
    Box<int> intBox = new Box<int>(42);
    Box<string> strBox = new Box<string>("SP Language");
    
    println("IntBox item: \${intBox.get()}");
    println("StrBox item: \${strBox.get()}");
    
    let echoed = identity<double>(3.14159);
    println("Echoed: \${echoed}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Zero-Cost Generics" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Zero-Cost Generics</h1>
      <p className="text-sm text-slate-300">
        SP implements generics using <strong>monomorphization</strong>. The compiler generates specialized, concrete LLVM machine code for each unique type instantiation (e.g., <code>Box&lt;int&gt;</code> and <code>Box&lt;string&gt;</code> become separate native symbols).
      </p>

      <CodeBlock code={code} filename="generics.sp" />

      <Callout type="tip" title="No Runtime Overhead">
        Because generic code is fully monomorphized at compile time, access to generic members incurs zero runtime boxing, zero casting penalties, and zero virtual lookup indirection.
      </Callout>

      <PrevNextNav
        prev={{ title: "OOP & Structs", href: "/syntax-guide/oop" }}
        next={{ title: "Lambdas & Higher-Order", href: "/syntax-guide/lambdas" }}
      />
    </div>
  );
}
