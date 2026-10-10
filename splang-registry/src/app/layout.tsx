import type { Metadata } from "next";
import { Inter, JetBrains_Mono } from "next/font/google";
import "./globals.css";
import { Navbar } from "@/components/layout/Navbar";
import { Footer } from "@/components/layout/Footer";

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
  metadataBase: new URL("https://register.splang.shantopaul.com"),
  title: {
    default: "SP Package Registry — Discover, install, and publish packages for SP",
    template: "%s | SP Package Registry",
  },
  description:
    "The official centralized package repository for the SP Programming Language. Find, inspect, and install verified community and standard libraries for SP 3.1.0 GA.",
  keywords: [
    "SP Language",
    "Package Registry",
    "SPM",
    "spm install",
    "spm add",
    "spm publish",
    "SP packages",
    "systems programming",
    "compiler",
  ],
  authors: [{ name: "Shanto Paul", url: "https://shantopaul.com" }],
  icons: {
    icon: [
      { url: "/brand/favicon.ico", sizes: "any" },
      { url: "/brand/favicon.svg", type: "image/svg+xml" },
    ],
    shortcut: "/brand/favicon.ico",
    apple: "/brand/app-icon-navy.png",
  },
  openGraph: {
    title: "SP Package Registry (v3.1.0 GA)",
    description: "Official package registry for the SP Programming Language ecosystem.",
    url: "https://register.splang.shantopaul.com",
    siteName: "SP Package Registry",
    images: [
      {
        url: "/brand/og-image.png",
        width: 1200,
        height: 630,
        alt: "SP Package Registry",
      },
    ],
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
    <html lang="en" className={`dark ${inter.variable} ${jetbrainsMono.variable}`}>
      <body className="min-h-screen bg-[#020D21] text-slate-100 flex flex-col font-sans antialiased selection:bg-[#00D1FF]/20 selection:text-[#00D1FF]">
        <Navbar />
        <main className="flex-1">{children}</main>
        <Footer />
      </body>
    </html>
  );
}
