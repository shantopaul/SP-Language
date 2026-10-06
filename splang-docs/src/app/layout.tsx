import type { Metadata } from "next";
import { Inter, JetBrains_Mono } from "next/font/google";
import "./globals.css";
import { DocsLayoutClient } from "@/components/layout/DocsLayoutClient";

const inter = Inter({
  subsets: ["latin"],
  display: "swap",
  variable: "--font-inter",
});

const jetbrainsMono = JetBrains_Mono({
  subsets: ["latin"],
  display: "swap",
  variable: "--font-jetbrains-mono",
});

export const metadata: Metadata = {
  metadataBase: new URL("https://docs.splang.shantopaul.com"),
  title: {
    default: "Documentation — SP Programming Language",
    template: "%s | SP Language Docs",
  },
  description:
    "Official documentation for the SP Programming Language. Syntax guides, standard library API reference, toolchain CLI manual, and language tutorials.",
  keywords: [
    "SP Documentation",
    "SP Syntax Guide",
    "std.io",
    "std.collections",
    "spc build",
    "spm package manager",
    "LLVM",
    "Shanto Paul",
  ],
  icons: {
    icon: "/logo.svg",
    shortcut: "/logo.svg",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className={`dark scroll-smooth ${inter.variable} ${jetbrainsMono.variable}`}>
      <body className="font-sans antialiased selection:bg-cyan-500/30 selection:text-cyan-200">
        <DocsLayoutClient>{children}</DocsLayoutClient>
      </body>
    </html>
  );
}
