export interface NavItem {
  title: string;
  href: string;
  badge?: string;
}

export interface NavSection {
  title: string;
  items: NavItem[];
}

export const DOCS_NAVIGATION: NavSection[] = [
  {
    title: "Getting Started",
    items: [
      { title: "Introduction & Overview", href: "/getting-started" },
      { title: "Installation", href: "/getting-started/installation" },
      { title: "First Program (Hello SP)", href: "/getting-started/first-program" },
      { title: "Project Scaffolding (spm)", href: "/getting-started/project-setup" },
    ],
  },
  {
    title: "Language Tour & Syntax",
    items: [
      { title: "Syntax Overview", href: "/syntax-guide" },
      { title: "Variables & Constants", href: "/syntax-guide/variables" },
      { title: "Primitive & Complex Types", href: "/syntax-guide/data-types" },
      { title: "Control Flow & Matching", href: "/syntax-guide/control-flow" },
      { title: "Functions & Closures", href: "/syntax-guide/functions" },
      { title: "OOP, Classes & Structs", href: "/syntax-guide/oop" },
      { title: "Zero-Cost Generics", href: "/syntax-guide/generics" },
      { title: "Lambdas & Higher-Order", href: "/syntax-guide/lambdas" },
      { title: "Null Safety & Operators", href: "/syntax-guide/null-safety" },
    ],
  },
  {
    title: "Standard Library (stdlib)",
    items: [
      { title: "Stdlib Overview", href: "/stdlib" },
      { title: "std.io (Console I/O)", href: "/stdlib/io" },
      { title: "std.string (String Utils)", href: "/stdlib/string" },
      { title: "std.math (Math & Trig)", href: "/stdlib/math" },
      { title: "std.collections (List, Map, Set)", href: "/stdlib/collections" },
      { title: "std.fs (File System)", href: "/stdlib/fs" },
      { title: "std.time (Clock & Duration)", href: "/stdlib/time" },
      { title: "std.os (Environment & System)", href: "/stdlib/os" },
      { title: "std.process (Execution & IPC)", href: "/stdlib/process" },
    ],
  },
  {
    title: "Developer Tooling",
    items: [
      { title: "Toolchain Overview", href: "/tools" },
      { title: "Compiler CLI (spc)", href: "/tools/compiler-cli" },
      { title: "Package Manager (spm)", href: "/tools/package-manager" },
      { title: "Code Formatter (spc fmt)", href: "/tools/formatter" },
      { title: "Native Test Runner (spc test)", href: "/tools/test-runner" },
      { title: "Doc Generator (spc doc)", href: "/tools/doc-generator" },
      { title: "Language Server (spls)", href: "/tools/lsp-server" },
      { title: "DAP Debugger (spd)", href: "/tools/debugger" },
      { title: "VS Code Extension", href: "/tools/vscode-extension" },
    ],
  },
  {
    title: "Performance & Architecture",
    items: [
      { title: "7-Language Benchmarks", href: "/benchmarks" },
    ],
  },
];
