import { Category } from "@/types/package";

export const CATEGORIES: Category[] = [
  {
    id: "web",
    name: "Web & Networking",
    description: "HTTP clients, microservice frameworks, WebSocket servers, and API toolkits.",
    iconName: "Globe",
  },
  {
    id: "cli",
    name: "CLI & Tooling",
    description: "Command-line parsers, terminal colorizers, progress bars, and shell interaction.",
    iconName: "Terminal",
  },
  {
    id: "utilities",
    name: "Utilities & Data",
    description: "JSON, TOML, serialization, logging, and data manipulation helpers.",
    iconName: "Wrench",
  },
  {
    id: "crypto",
    name: "Security & Crypto",
    description: "Cryptographic hashes (SHA-256), encryption algorithms, and secure tokens.",
    iconName: "Shield",
  },
  {
    id: "math",
    name: "Math & Science",
    description: "Linear algebra, matrix operations, statistical computing, and numerical kernels.",
    iconName: "Calculator",
  },
  {
    id: "database",
    name: "Database & Storage",
    description: "SQLite drivers, key-value stores, and database connection pooling.",
    iconName: "Database",
  },
  {
    id: "testing",
    name: "Testing & Profiling",
    description: "Assertion libraries, micro-benchmarking harnesses, and mock frameworks.",
    iconName: "CheckCircle",
  },
];
