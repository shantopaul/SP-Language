import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.fs Module Reference",
  description: "Cross-platform filesystem operations, file reading, and directory access in SP.",
};

export default function StdFsPage() {
  const exampleCode = `import std.io;
import std.fs;

fn main() -> void {
    let path = "server.log";
    fs.writeText(path, "INFO: Starting SP microservice on port 8080\\n");
    
    if (fs.exists(path)) {
        let content = fs.readText(path);
        println("Log contents:\\n\${content}");
        fs.delete(path);
    }
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.fs" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.fs Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.fs</code> module provides synchronous and asynchronous file system operations with cross-platform path handling.
      </p>

      <CodeBlock code={exampleCode} filename="fs_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported Functions</h2>

      <ApiCard
        name="fs.readText"
        signature="fn fs.readText(string path) -> string"
        description="Reads the entire contents of a file as a UTF-8 string."
        parameters={[{ name: "path", type: "string", description: "Path to the target file." }]}
        returns="string"
      />

      <ApiCard
        name="fs.writeText"
        signature="fn fs.writeText(string path, string content) -> bool"
        description="Overwrites or creates a file with the provided text content."
        parameters={[
          { name: "path", type: "string", description: "Target file path." },
          { name: "content", type: "string", description: "Data to write." },
        ]}
        returns="bool (true if successful)"
      />

      <ApiCard
        name="fs.exists"
        signature="fn fs.exists(string path) -> bool"
        description="Returns true if a file or directory exists at path."
        parameters={[{ name: "path", type: "string", description: "File path." }]}
        returns="bool"
      />

      <ApiCard
        name="fs.delete"
        signature="fn fs.delete(string path) -> bool"
        description="Deletes the file at the given path."
        parameters={[{ name: "path", type: "string", description: "File to delete." }]}
        returns="bool"
      />

      <PrevNextNav
        prev={{ title: "std.collections Reference", href: "/stdlib/collections" }}
        next={{ title: "std.time Reference", href: "/stdlib/time" }}
      />
    </div>
  );
}
