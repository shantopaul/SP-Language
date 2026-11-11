import type { Metadata } from "next";
import { Inter, JetBrains_Mono } from "next/font/google";
import "./globals.css";
import { DocsLayoutClient } from "@/components/layout/DocsLayoutClient";
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
  metadataBase: new URL("https://docs.splang.shantopaul.com"),
  title: {
    default: "Documentation — SP Programming Language",
    template: "%s | SP Language Docs",
  },
  description:
    "Official developer documentation for the SP Programming Language (v4.0.0 GA). Complete syntax guides, 23 standard library modules, std.web framework, LLVM compiler toolchain, and package manager.",
  keywords: [
    "SP Programming Language Documentation",
    "SP Docs",
    "std.web",
    "std.io",
    "std.db",
    "std.collections",
    "spc compiler",
    "spc upgrade",
    "spm package manager",
    "LLVM",
    "Shanto Paul",
  ],
  icons: {
    icon: [
      { url: "/brand/favicon.ico", sizes: "any" },
      { url: "/brand/favicon.svg", type: "image/svg+xml" },
    ],
    apple: "/brand/app-icon-navy.png",
    shortcut: "/brand/favicon.ico",
  },
  openGraph: {
    title: "SP Programming Language Documentation (v4.0.0 GA)",
    description:
      "Comprehensive syntax guides, 23 standard library modules, std.web platform, and native LLVM toolchain manuals.",
    url: "https://docs.splang.shantopaul.com",
    siteName: "SP Language Docs",
    images: [
      {
        url: "/brand/og-image.png",
        width: 1200,
        height: 630,
        alt: "SP Programming Language Documentation",
      },
    ],
    locale: "en_US",
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "SP Programming Language Documentation (v4.0.0 GA)",
    description:
      "Syntax guides, 23 standard library modules, std.web microservices, and native LLVM toolchain manuals.",
    images: ["/brand/og-image.png"],
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html
      lang="en"
      className={`dark scroll-smooth ${inter.variable} ${jetbrainsMono.variable}`}
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
      <body className="font-sans antialiased bg-slate-50 dark:bg-[#020D21] text-slate-900 dark:text-slate-100 selection:bg-[#0057FF]/30 selection:text-[#00D1FF] min-h-screen transition-colors duration-200">
        <ThemeProvider>
          <DocsLayoutClient>{children}</DocsLayoutClient>
        </ThemeProvider>
      </body>
    </html>
  );
}
