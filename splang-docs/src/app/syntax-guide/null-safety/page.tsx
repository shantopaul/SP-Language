import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Null Safety & Operators",
  description: "Compile-time null safety, safe navigation, and coalescing in SP.",
};

export default function NullSafetyPage() {
  const code = `import std.io;

class Profile {
    public string bio;
    public Profile(string bio) {
        this.bio = bio;
    }
}

class User {
    public Profile? profile; // Nullable type
    public User(Profile? p) {
        this.profile = p;
    }
}

fn main() -> void {
    User u1 = new User(null);
    User u2 = new User(new Profile("Systems Architect"));
    
    // 1. Safe Navigation Operator (?.)
    string? bio1 = u1.profile?.bio;
    string? bio2 = u2.profile?.bio;
    
    // 2. Null Coalescing Operator (??)
    string displayBio = bio1 ?? "No biography provided";
    
    println("User 1 Bio: \${displayBio}");
    println("User 2 Bio: \${bio2 ?? 'Default'}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Null Safety & Operators" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Null Safety in SP</h1>
      <p className="text-sm text-slate-300">
        In SP, standard types cannot contain <code>null</code> unless explicitly declared as nullable with a question mark (<code>T?</code>). This eliminates billion-dollar null pointer exceptions at compile time.
      </p>

      <CodeBlock code={code} filename="null_safety.sp" />

      <h2 className="text-xl font-bold text-white pt-2">Safe Operators</h2>
      <div className="space-y-4 text-xs sm:text-sm">
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h3 className="font-mono text-cyan-400 font-bold mb-1">Safe Navigation: a?.b</h3>
          <p className="text-slate-400">
            If <code>a</code> is null, the expression short-circuits and evaluates to <code>null</code> without crashing.
          </p>
        </div>
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h3 className="font-mono text-emerald-400 font-bold mb-1">Null Coalescing: a ?? b</h3>
          <p className="text-slate-400">
            If <code>a</code> is not null, yields <code>a</code>; otherwise evaluates and returns fallback default <code>b</code>.
          </p>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Lambdas & Closures", href: "/syntax-guide/lambdas" }}
        next={{ title: "Standard Library Overview", href: "/stdlib" }}
      />
    </div>
  );
}
