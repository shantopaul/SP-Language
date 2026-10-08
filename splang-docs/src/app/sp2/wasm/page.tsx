import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Globe, Shield, Zap, Terminal } from "lucide-react";

export const metadata = {
  title: "First-Class WebAssembly (wasm32-wasi)",
  description: "Cross-compile SP 2.0 applications directly into WebAssembly (WASI) binaries for edge runtimes, browser sandboxes, and cloud execution.",
};

export default function Sp2WasmPage() {
  const compileCmd = `# Cross-compile an SP program to WebAssembly WASI binary
spc build src/main.sp --target wasm32-wasi -O3 -o dist/app.wasm

# Inspect binary size (ultra-lean with zero bundled GC runtime)
ls -lh dist/app.wasm`;

  const runWasiCmd = `# Execute via Wasmtime CLI
wasmtime dist/app.wasm

# Execute via Wasmer CLI
wasmer run dist/app.wasm

# Execute in Node.js 18+ WASI runtime
node --experimental-wasi-unstable-preview1 runner.mjs`;

  const nodeRunnerCode = `import { readFile } from "node:fs/promises";
import { WASI } from "node:wasi";
import { argv, env } from "node:process";

const wasi = new WASI({ version: "preview1", args: argv, env });
const wasmBytes = await readFile("./dist/app.wasm");
const { instance } = await WebAssembly.instantiate(wasmBytes, wasi.getImportObject());

wasi.start(instance);`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 2.0 Evolution", href: "/sp2/overview" },
          { title: "WebAssembly" },
        ]}
      />

      <div>
        <h1 className="text-3xl font-extrabold text-white">First-Class WebAssembly (`wasm32-wasi`)</h1>
        <p className="mt-3 text-sm text-slate-300 leading-relaxed">
          SP 2.0 introduces native compilation to WebAssembly through the <code>wasm32-wasi</code> target triple. Build portable, sandboxed, high-performance binaries ready to deploy across edge workers, browser contexts, microVMs, and containerless cloud environments.
        </p>
      </div>

      <Callout type="tip" title="No Garbage Collector Overhead in WASM">
        Unlike Go or Java which must bundle multi-megabyte garbage collector runtimes into WebAssembly binaries, SP uses <strong>Deterministic ARC (Automatic Reference Counting)</strong>. SP WASM binaries are minimal in footprint (often under 50KB compressed) and boast sub-millisecond cold start times.
      </Callout>

      <h2 className="text-xl font-bold text-white pt-2">Compiling to WebAssembly</h2>
      <p className="text-sm text-slate-300">
        Pass the <code>--target wasm32-wasi</code> flag to <code>spc build</code>:
      </p>
      <CodeBlock code={compileCmd} language="bash" />

      <h2 className="text-xl font-bold text-white pt-4">Running in WASI Runtimes</h2>
      <p className="text-sm text-slate-300">
        Deploy and execute on standard WASI runtimes without any native host dependencies:
      </p>
      <CodeBlock code={runWasiCmd} language="bash" />

      <h2 className="text-xl font-bold text-white pt-4">Running via Node.js WASI Runner</h2>
      <p className="text-sm text-slate-300">
        Embed your compiled SP WASM modules into existing JavaScript / TypeScript backend microservices:
      </p>
      <CodeBlock code={nodeRunnerCode} filename="runner.mjs" language="javascript" />

      <h2 className="text-xl font-bold text-white pt-4">Supported WASI Capabilities</h2>
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 text-xs sm:text-sm">
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h4 className="font-bold text-white mb-1.5 flex items-center gap-1.5">
            <Terminal className="w-4 h-4 text-emerald-400" />
            <span>Standard I/O &amp; Streams</span>
          </h4>
          <p className="text-slate-400 leading-relaxed">
            Full support for <code>std.io.println</code>, <code>std.io.readLine</code>, formatted logging, and file descriptors via WASI preview1.
          </p>
        </div>

        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h4 className="font-bold text-white mb-1.5 flex items-center gap-1.5">
            <Shield className="w-4 h-4 text-blue-400" />
            <span>Sandboxed Filesystem &amp; Clocks</span>
          </h4>
          <p className="text-slate-400 leading-relaxed">
            Safe, pre-opened directories for <code>std.fs</code> read/write operations and nanosecond precision clocks via <code>std.time</code>.
          </p>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Compile-Time Execution (comptime)", href: "/sp2/comptime" }}
        next={{ title: "Structured Coroutines (async/await)", href: "/sp2/async-await" }}
      />
    </div>
  );
}
