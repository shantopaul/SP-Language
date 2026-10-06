"use client";

import React, { useState } from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import {
  BookOpen,
  Boxes,
  Code2,
  Download,
  Github,
  Menu,
  Sparkles,
  X,
  ExternalLink,
  Flame,
} from "lucide-react";
import { Badge } from "@/components/ui/Badge";
import { Button } from "@/components/ui/Button";

export const Navbar: React.FC = () => {
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);
  const pathname = usePathname();

  const navLinks = [
    { name: "Documentation", href: "https://docs.splang.shantopaul.com", external: true, icon: BookOpen },
    { name: "Registry", href: "https://registry.splang.shantopaul.com", external: true, icon: Boxes },
    { name: "Benchmarks", href: "/benchmarks", icon: Flame },
    { name: "Downloads", href: "/downloads", icon: Download },
    { name: "Community", href: "/community", icon: Code2 },
  ];

  return (
    <header className="sticky top-0 z-50 w-full border-b border-slate-800/80 bg-slate-950/80 backdrop-blur-xl">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          {/* Brand Logo & Version */}
          <div className="flex items-center gap-3">
            <Link href="/" className="flex items-center gap-2.5 group">
              <div className="w-9 h-9 rounded-xl bg-gradient-to-tr from-emerald-500 via-cyan-500 to-blue-500 p-0.5 shadow-md shadow-cyan-500/20 group-hover:shadow-cyan-500/40 transition-shadow">
                <div className="w-full h-full bg-slate-950 rounded-[10px] flex items-center justify-center font-bold text-transparent bg-clip-text bg-gradient-to-tr from-emerald-400 to-cyan-400 text-lg">
                  SP
                </div>
              </div>
              <div className="flex flex-col">
                <span className="font-bold text-white text-base tracking-tight group-hover:text-cyan-400 transition-colors">
                  SP Language
                </span>
                <span className="text-[10px] font-mono text-slate-400 -mt-1">
                  splang.shantopaul.com
                </span>
              </div>
            </Link>
            <Badge variant="success" className="hidden sm:inline-flex ml-2">
              <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
              v1.0.0 GA
            </Badge>
          </div>

          {/* Desktop Navigation Links */}
          <nav className="hidden md:flex items-center gap-1">
            {navLinks.map((link) => {
              const Icon = link.icon;
              const isActive = pathname === link.href;

              if (link.external) {
                return (
                  <a
                    key={link.name}
                    href={link.href}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-sm font-medium text-slate-300 hover:text-white hover:bg-slate-900/80 transition-colors group"
                  >
                    <Icon className="w-4 h-4 text-slate-400 group-hover:text-cyan-400 transition-colors" />
                    <span>{link.name}</span>
                    <ExternalLink className="w-3 h-3 text-slate-500 group-hover:text-slate-400" />
                  </a>
                );
              }

              return (
                <Link
                  key={link.name}
                  href={link.href}
                  className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-sm font-medium transition-colors ${
                    isActive
                      ? "text-cyan-400 bg-cyan-950/40 border border-cyan-800/40"
                      : "text-slate-300 hover:text-white hover:bg-slate-900/80"
                  }`}
                >
                  <Icon className="w-4 h-4 text-slate-400" />
                  <span>{link.name}</span>
                </Link>
              );
            })}
          </nav>

          {/* Right Action Buttons */}
          <div className="hidden sm:flex items-center gap-3">
            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center gap-2 px-3 py-1.5 rounded-lg border border-slate-800 bg-slate-900/60 text-slate-300 hover:text-white hover:border-slate-700 transition-all text-sm font-medium"
            >
              <Github className="w-4 h-4" />
              <span>GitHub</span>
            </a>
            <a
              href="https://docs.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
            >
              <Button size="sm" variant="primary">
                <Sparkles className="w-3.5 h-3.5" />
                <span>Get Started</span>
              </Button>
            </a>
          </div>

          {/* Mobile menu button */}
          <div className="md:hidden flex items-center">
            <button
              onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
              className="p-2 rounded-lg text-slate-400 hover:text-white hover:bg-slate-900 focus:outline-none"
              aria-label="Toggle menu"
            >
              {mobileMenuOpen ? <X className="w-6 h-6" /> : <Menu className="w-6 h-6" />}
            </button>
          </div>
        </div>
      </div>

      {/* Mobile Drawer */}
      {mobileMenuOpen && (
        <div className="md:hidden border-b border-slate-800 bg-slate-950/95 px-4 pt-2 pb-6 space-y-2">
          {navLinks.map((link) => (
            <a
              key={link.name}
              href={link.href}
              target={link.external ? "_blank" : undefined}
              rel={link.external ? "noopener noreferrer" : undefined}
              onClick={() => setMobileMenuOpen(false)}
              className="flex items-center justify-between px-3 py-2 rounded-lg text-base font-medium text-slate-300 hover:text-white hover:bg-slate-900"
            >
              <span className="flex items-center gap-2">
                <link.icon className="w-5 h-5 text-cyan-400" />
                {link.name}
              </span>
              {link.external && <ExternalLink className="w-4 h-4 text-slate-500" />}
            </a>
          ))}
          <div className="pt-4 flex flex-col gap-2">
            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center justify-center gap-2 w-full py-2.5 rounded-lg border border-slate-800 text-slate-300 text-sm font-medium"
            >
              <Github className="w-4 h-4" />
              <span>View on GitHub</span>
            </a>
            <a
              href="https://docs.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="w-full"
            >
              <Button className="w-full justify-center">Get Started</Button>
            </a>
          </div>
        </div>
      )}
    </header>
  );
};
