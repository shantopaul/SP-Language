"use client";

import React, { useState } from "react";
import { Header } from "./Header";
import { Sidebar } from "./Sidebar";
import { DocsFooter } from "./Footer";

export const DocsLayoutClient: React.FC<{ children: React.ReactNode }> = ({
  children,
}) => {
  const [sidebarOpen, setSidebarOpen] = useState(false);

  return (
    <div className="min-h-screen bg-slate-50 dark:bg-[#020D21] text-slate-900 dark:text-slate-100 flex flex-col relative selection:bg-[#0057FF]/30 selection:text-[#00D1FF] transition-colors duration-200">
      {/* Ambient background glow */}
      <div className="fixed top-0 left-1/4 w-[600px] h-[300px] bg-gradient-to-b from-[#0057FF]/10 to-transparent rounded-full blur-[120px] pointer-events-none -z-10" />
      <div className="fixed bottom-0 right-1/4 w-[500px] h-[250px] bg-gradient-to-t from-[#00D1FF]/5 to-transparent rounded-full blur-[120px] pointer-events-none -z-10" />

      <Header onToggleSidebar={() => setSidebarOpen((prev) => !prev)} />

      <div className="flex-1 flex max-w-7xl mx-auto w-full px-4 sm:px-6 lg:px-8">
        <Sidebar isOpen={sidebarOpen} onClose={() => setSidebarOpen(false)} />
        <main className="flex-1 py-8 px-0 lg:px-10 min-w-0 max-w-4xl">
          {children}
          <DocsFooter />
        </main>
      </div>
    </div>
  );
};
