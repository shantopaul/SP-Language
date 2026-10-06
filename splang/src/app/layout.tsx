import type { Metadata } from "next";
import "./globals.css";
import { Navbar } from "@/components/layout/Navbar";
import { Footer } from "@/components/layout/Footer";

export const metadata: Metadata = {
  metadataBase: new URL("https://splang.shantopaul.com"),
  title: {
    default: "SP Programming Language — High-Performance Systems & Applications",
    template: "%s | SP Programming Language",
  },
  description:
    "The modern, high-performance systems and application programming language with LLVM native compilation, safe memory ergonomics, zero-cost generics, and a unified toolchain.",
  keywords: [
    "SP Language",
    "Programming Language",
    "LLVM",
    "Systems Programming",
    "High Performance",
    "Native Compiler",
    "Package Manager",
    "Type Safe",
    "Shanto Paul",
  ],
  authors: [{ name: "Shanto Paul", url: "https://shantopaul.com" }],
  creator: "Shanto Paul",
  openGraph: {
    type: "website",
    locale: "en_US",
    url: "https://splang.shantopaul.com",
    siteName: "SP Programming Language",
    title: "SP Programming Language — Modern, High-Performance Systems & Application Language",
    description:
      "Bare-metal LLVM native compilation meets modern developer ergonomics. Fast, safe, and productive.",
  },
  twitter: {
    card: "summary_large_image",
    title: "SP Programming Language",
    description:
      "Modern, high-performance systems language with native LLVM backend and built-in package manager.",
    creator: "@shantopaul",
  },
  icons: {
    icon: "/logo.svg",
    shortcut: "/logo.svg",
    apple: "/logo.svg",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className="dark scroll-smooth">
      <body className="min-h-screen bg-[#020617] text-slate-100 flex flex-col antialiased selection:bg-cyan-500/30 selection:text-cyan-200">
        <Navbar />
        <main className="flex-1">{children}</main>
        <Footer />
      </body>
    </html>
  );
}
