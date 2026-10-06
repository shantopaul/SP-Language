import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.os Module Reference",
  description: "Environment variables, CLI arguments, and platform inspection in SP.",
};

export default function StdOsPage() {
  const exampleCode = `import std.io;
import std.os;

fn main() -> void {
    let platform = os.platform();
    let user = os.env("USER") ?? os.env("USERNAME") ?? "Unknown";
    
    println("Platform: \${platform}");
    println("Active User: \${user}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.os" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.os Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.os</code> module enables inspecting environment variables, host operating system properties, and controlling process termination.
      </p>

      <CodeBlock code={exampleCode} filename="os_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported Functions</h2>

      <ApiCard
        name="os.platform"
        signature="fn os.platform() -> string"
        description="Returns the host operating system string ('windows', 'linux', or 'macos')."
        returns="string"
      />

      <ApiCard
        name="os.env"
        signature="fn os.env(string name) -> string?"
        description="Retrieves the value of an environment variable, returning null if unset."
        parameters={[{ name: "name", type: "string", description: "Environment variable key." }]}
        returns="string?"
      />

      <ApiCard
        name="os.exit"
        signature="fn os.exit(int code) -> void"
        description="Terminates the host process immediately with the specified exit status."
        parameters={[{ name: "code", type: "int", description: "Process return code." }]}
        returns="void"
      />

      <PrevNextNav
        prev={{ title: "std.time Reference", href: "/stdlib/time" }}
        next={{ title: "std.process Reference", href: "/stdlib/process" }}
      />
    </div>
  );
}
