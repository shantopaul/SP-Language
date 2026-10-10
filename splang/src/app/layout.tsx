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
  metadataBase: new URL("https://splang.shantopaul.com"),
  title: {
    default: "SP Programming Language — Modern High-Performance Systems & Backend Platform",
    template: "%s | SP Programming Language",
  },
  description:
    "The modern, high-performance systems and backend platform language with LLVM native compilation, deterministic ARC memory safety, 23 standard library modules, and 185,000 req/s enterprise web performance.",
  keywords: [
    "SP Language",
    "Programming Language",
    "LLVM",
    "Systems Programming",
    "High Performance",
    "Native Compiler",
    "Package Manager",
    "Type Safe",
    "std.web",
    "Shanto Paul",
  ],
  authors: [{ name: "Shanto Paul", url: "https://shantopaul.com" }],
  creator: "Shanto Paul",
  openGraph: {
    type: "website",
    locale: "en_US",
    url: "https://splang.shantopaul.com",
    siteName: "SP Programming Language",
    title: "SP Programming Language — Modern, High-Performance Systems & Backend Platform",
    description:
      "Bare-metal LLVM native compilation meets deterministic ARC memory safety and an enterprise web platform delivering 185,000 req/s. Fast, safe, and productive.",
    images: [
      {
        url: "/brand/og-image.png",
        width: 1200,
        height: 630,
        alt: "SP Programming Language — High-Performance Systems Platform",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "SP Programming Language",
    description:
      "Modern, high-performance systems language with native LLVM backend and enterprise web framework.",
    creator: "@shantopaul",
    images: ["/brand/og-image.png"],
  },
  icons: {
    icon: [
      { url: "/favicon.ico" },
      { url: "/favicon.svg", type: "image/svg+xml" },
    ],
    shortcut: "/favicon.ico",
    apple: "/brand/app-icon-navy.png",
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
      <body className="min-h-screen bg-slate-50 dark:bg-[#020D21] text-slate-900 dark:text-slate-100 flex flex-col font-sans antialiased selection:bg-cyan-500/30 selection:text-white transition-colors duration-200">
        <ThemeProvider>
          <Navbar />
          <main className="flex-1">{children}</main>
          <Footer />
        </ThemeProvider>
      </body>
    </html>
  );
}
