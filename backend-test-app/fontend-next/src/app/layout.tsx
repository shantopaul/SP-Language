import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "SP Lang Fullstack CRUD Dashboard",
  description: "Next.js App Router fullstack application running on SP Lang native backend engine",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className="dark">
      <body className="antialiased selection:bg-indigo-500 selection:text-white">
        {children}
      </body>
    </html>
  );
}
