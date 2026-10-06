import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.collections Module Reference",
  description: "Dynamic collections (List, Map, Set) and iterator utilities in SP.",
};

export default function StdCollectionsPage() {
  const exampleCode = `import std.io;
import std.collections;

fn main() -> void {
    // 1. Dynamic List
    List<string> cities = new List<string>();
    cities.add("Tokyo");
    cities.add("London");
    cities.add("New York");
    
    // 2. Hash Map
    Map<string, int> pop = new Map<string, int>();
    pop.set("Tokyo", 14000000);
    pop.set("London", 9000000);
    
    println("Cities count: \${cities.size()}");
    println("Tokyo population: \${pop.get('Tokyo')}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.collections" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.collections Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.collections</code> module provides dynamically resizable data structures: <code>List&lt;T&gt;</code>, <code>Map&lt;K, V&gt;</code>, and <code>Set&lt;T&gt;</code>.
      </p>

      <CodeBlock code={exampleCode} filename="collections_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Collection Primitives</h2>

      <ApiCard
        name="List<T>"
        signature="class List<T> {&#10;    fn add(T item) -> void;&#10;    fn get(int index) -> T;&#10;    fn size() -> int;&#10;    fn clear() -> void;&#10;}"
        description="Dynamic resizable vector with amortized O(1) appending and contiguous memory layout."
      />

      <ApiCard
        name="Map<K, V>"
        signature="class Map<K, V> {&#10;    fn set(K key, V val) -> void;&#10;    fn get(K key) -> V?;&#10;    fn containsKey(K key) -> bool;&#10;    fn remove(K key) -> void;&#10;}"
        description="High-performance hash table with robin-hood hashing and linear probing."
      />

      <PrevNextNav
        prev={{ title: "std.math Reference", href: "/stdlib/math" }}
        next={{ title: "std.fs Reference", href: "/stdlib/fs" }}
      />
    </div>
  );
}
