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
    <div className="min-h-screen bg-[#020617] text-slate-100 flex flex-col">
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
