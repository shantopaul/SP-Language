export interface LanguageResult {
  language: string;
  runtimeMs: number;
  memoryMb: number;
  compiler: string;
  speedupVsPython: string;
  notes?: string;
  isSP?: boolean;
}

export interface BenchmarkWorkload {
  id: string;
  name: string;
  description: string;
  results: LanguageResult[];
}

export const BENCHMARKS_DATA: BenchmarkWorkload[] = [
  {
    id: "fibonacci",
    name: "Recursive Fibonacci (n=40)",
    description: "Evaluates function call overhead, stack frame management, and recursion speed without caching.",
    results: [
      { language: "SP", runtimeMs: 312, memoryMb: 14.2, compiler: "spc -O3", speedupVsPython: "62.5x", isSP: true },
      { language: "C", runtimeMs: 304, memoryMb: 2.1, compiler: "gcc -O3", speedupVsPython: "64.1x" },
      { language: "C++", runtimeMs: 308, memoryMb: 3.4, compiler: "clang++ -O3", speedupVsPython: "63.3x" },
      { language: "Rust", runtimeMs: 318, memoryMb: 4.2, compiler: "rustc --release", speedupVsPython: "61.3x" },
      { language: "Java", runtimeMs: 512, memoryMb: 48.0, compiler: "OpenJDK 21 (HotSpot)", speedupVsPython: "38.1x" },
      { language: "Node.js", runtimeMs: 980, memoryMb: 38.5, compiler: "Node.js v20 (V8)", speedupVsPython: "19.9x" },
      { language: "Python", runtimeMs: 19500, memoryMb: 16.0, compiler: "CPython 3.12", speedupVsPython: "1.0x" },
    ],
  },
  {
    id: "quicksort",
    name: "Quicksort (1,000,000 Elements)",
    description: "Evaluates raw memory throughput, branch prediction, array indexing, and in-place swapping.",
    results: [
      { language: "SP", runtimeMs: 82, memoryMb: 22.0, compiler: "spc -O3", speedupVsPython: "29.3x", isSP: true },
      { language: "C", runtimeMs: 78, memoryMb: 12.0, compiler: "gcc -O3", speedupVsPython: "30.8x" },
      { language: "C++", runtimeMs: 79, memoryMb: 12.5, compiler: "clang++ -O3", speedupVsPython: "30.4x" },
      { language: "Rust", runtimeMs: 84, memoryMb: 13.0, compiler: "rustc --release", speedupVsPython: "28.6x" },
      { language: "Java", runtimeMs: 142, memoryMb: 72.0, compiler: "OpenJDK 21 (HotSpot)", speedupVsPython: "16.9x" },
      { language: "Node.js", runtimeMs: 240, memoryMb: 68.0, compiler: "Node.js v20 (V8)", speedupVsPython: "10.0x" },
      { language: "Python", runtimeMs: 2400, memoryMb: 45.0, compiler: "CPython 3.12", speedupVsPython: "1.0x" },
    ],
  },
  {
    id: "hashmap",
    name: "Hash Map (1,000,000 Insert & Lookup)",
    description: "Evaluates hash collision handling, dynamic bucket resizing, pointer dereferencing, and allocator throughput.",
    results: [
      { language: "SP", runtimeMs: 185, memoryMb: 48.0, compiler: "spc -O3", speedupVsPython: "7.7x", isSP: true },
      { language: "C", runtimeMs: 162, memoryMb: 36.0, compiler: "gcc -O3", speedupVsPython: "8.8x" },
      { language: "C++", runtimeMs: 170, memoryMb: 38.0, compiler: "clang++ -O3", speedupVsPython: "8.4x" },
      { language: "Rust", runtimeMs: 178, memoryMb: 42.0, compiler: "rustc --release", speedupVsPython: "8.0x" },
      { language: "Java", runtimeMs: 295, memoryMb: 128.0, compiler: "OpenJDK 21 (HotSpot)", speedupVsPython: "4.8x" },
      { language: "Node.js", runtimeMs: 460, memoryMb: 110.0, compiler: "Node.js v20 (V8)", speedupVsPython: "3.1x" },
      { language: "Python", runtimeMs: 1420, memoryMb: 95.0, compiler: "CPython 3.12", speedupVsPython: "1.0x" },
    ],
  },
  {
    id: "binarytrees",
    name: "Binary Trees (Depth 16)",
    description: "Classic GC/allocator benchmark allocating, traversing, and deallocating deep recursive trees.",
    results: [
      { language: "SP", runtimeMs: 410, memoryMb: 64.0, compiler: "spc -O3", speedupVsPython: "11.7x", isSP: true },
      { language: "C", runtimeMs: 345, memoryMb: 44.0, compiler: "gcc -O3", speedupVsPython: "13.9x" },
      { language: "C++", runtimeMs: 360, memoryMb: 46.0, compiler: "clang++ -O3", speedupVsPython: "13.3x" },
      { language: "Rust", runtimeMs: 395, memoryMb: 48.0, compiler: "rustc --release", speedupVsPython: "12.2x" },
      { language: "Java", runtimeMs: 580, memoryMb: 210.0, compiler: "OpenJDK 21 (HotSpot)", speedupVsPython: "8.3x" },
      { language: "Node.js", runtimeMs: 820, memoryMb: 195.0, compiler: "Node.js v20 (V8)", speedupVsPython: "5.9x" },
      { language: "Python", runtimeMs: 4800, memoryMb: 130.0, compiler: "CPython 3.12", speedupVsPython: "1.0x" },
    ],
  },
  {
    id: "mandelbrot",
    name: "Mandelbrot (1,000 x 1,000 Fractal)",
    description: "Evaluates double-precision floating point math, SIMD vectorization potential, and tight computational loops.",
    results: [
      { language: "SP", runtimeMs: 245, memoryMb: 16.0, compiler: "spc -O3", speedupVsPython: "33.5x", isSP: true },
      { language: "C", runtimeMs: 232, memoryMb: 2.8, compiler: "gcc -O3", speedupVsPython: "35.3x" },
      { language: "C++", runtimeMs: 236, memoryMb: 3.6, compiler: "clang++ -O3", speedupVsPython: "34.7x" },
      { language: "Rust", runtimeMs: 240, memoryMb: 4.8, compiler: "rustc --release", speedupVsPython: "34.2x" },
      { language: "Java", runtimeMs: 390, memoryMb: 52.0, compiler: "OpenJDK 21 (HotSpot)", speedupVsPython: "21.0x" },
      { language: "Node.js", runtimeMs: 650, memoryMb: 42.0, compiler: "Node.js v20 (V8)", speedupVsPython: "12.6x" },
      { language: "Python", runtimeMs: 8200, memoryMb: 18.0, compiler: "CPython 3.12", speedupVsPython: "1.0x" },
    ],
  },
];
