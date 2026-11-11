import type { Metadata } from "next";
import { Inter, JetBrains_Mono } from "next/font/google";
import "./globals.css";
import { Navbar } from "@/components/layout/Navbar";
import { Footer } from "@/components/layout/Footer";
import { ThemeProvider } from "@/components/theme/ThemeProvider";

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
    "The official centralized package repository for the SP Programming Language. Find, inspect, and install verified community and standard libraries for SP 4.0.0 GA.",
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
    title: "SP Package Registry (v4.0.0 GA)",
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
    <html
      lang="en"
      className={`dark ${inter.variable} ${jetbrainsMono.variable}`}
      suppressHydrationWarning
    >
      <head>
        <script
          dangerouslySetInnerHTML={{
            __html: `
              (function() {
                try {
                  var saved = localStorage.getItem('sp_theme');
                  var prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
                  var isDark = saved === 'dark' || (saved === 'system' || !saved ? prefersDark : false);
                  if (isDark) {
                    document.documentElement.classList.add('dark');
                    document.documentElement.classList.remove('light');
                  } else {
                    document.documentElement.classList.remove('dark');
                    document.documentElement.classList.add('light');
                  }
                } catch (e) {}
              })();
            `,
          }}
        />
      </head>
      <body className="min-h-screen bg-slate-50 dark:bg-[#020D21] text-slate-900 dark:text-slate-100 flex flex-col font-sans antialiased selection:bg-[#00D1FF]/20 selection:text-[#00D1FF] transition-colors duration-200">
        <ThemeProvider>
          <Navbar />
          <main className="flex-1">{children}</main>
          <Footer />
        </ThemeProvider>
      </body>
    </html>
  );
}
