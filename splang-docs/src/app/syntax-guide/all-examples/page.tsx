import React from "react";
import type { Metadata } from "next";
import { SyntaxGuideClient } from "./SyntaxGuideClient";

export const metadata: Metadata = {
  title: "Complete Syntax Guide (1–100 Examples)",
  description:
    "Complete reference catalog of all 100 SP language syntax features, patterns, standard libraries, systems programming techniques, and real-world microservices with compiled outputs.",
  keywords: [
    "SP Language Syntax",
    "SP 100 Examples",
    "SP syntax cheat sheet",
    "SP programming examples",
    "spc run",
    "spc build",
  ],
};

export default function AllExamplesPage() {
  return <SyntaxGuideClient />;
}
