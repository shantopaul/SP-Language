import type { Metadata } from "next";
import "./globals.css";
import { Navbar } from "@/components/layout/Navbar";
import { Footer } from "@/components/layout/Footer";

export const metadata: Metadata = {
  metadataBase: new URL("https://registry.splang.shantopaul.com"),
  title: {
    default: "SP Package Registry — Discover, install, and publish packages for SP",
    template: "%s | SP Package Registry",
  },
  description:
    "The official centralized package repository for the SP Programming Language. Find, inspect, and install verified community and standard libraries for SP 1.0.",
  keywords: [
    "SP Language",
    "Package Registry",
    "SPM",
    "spm install",
    "spm add",
    "SP packages",
    "systems programming",
    "compiler",
  ],
  authors: [{ name: "Shanto Paul", url: "https://shantopaul.com" }],
  icons: {
    icon: "/logo.svg",
    shortcut: "/logo.svg",
    apple: "/logo.svg",
  },
  openGraph: {
    title: "SP Package Registry",
    description: "Official package registry for the SP Programming Language ecosystem.",
    url: "https://registry.splang.shantopaul.com",
    siteName: "SP Package Registry",
    locale: "en_US",
    type: "website",
  },
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <body className="min-h-screen bg-slate-950 text-slate-100 flex flex-col antialiased selection:bg-cyan-500/20 selection:text-cyan-300">
        <Navbar />
        <main className="flex-1">{children}</main>
        <Footer />
      </body>
    </html>
  );
}
