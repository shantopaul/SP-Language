"use client";

import React, { useState } from "react";
import Link from "next/link";
import Image from "next/image";
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
    <header className="sticky top-0 z-50 w-full border-b border-[#0A1F44] bg-[#020D21]/85 backdrop-blur-xl">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          {/* Official Brand Logo & Version */}
          <div className="flex items-center gap-3">
            <Link href="/" className="flex items-center gap-3 group">
              <div className="relative w-9 h-9 rounded-xl p-0.5 shadow-md shadow-blue-500/20 group-hover:shadow-cyan-400/35 transition-all">
                <Image
                  src="/brand/brand-emblem-navy.png"
                  alt="SP Language Emblem"
                  width={36}
                  height={36}
                  className="rounded-lg object-contain"
                  priority
                />
              </div>
              <div className="flex flex-col">
                <div className="font-extrabold text-white text-base tracking-tight leading-none group-hover:text-cyan-300 transition-colors flex items-center gap-1">
                  <span>SP</span>
                  <span className="text-[#00D1FF]">Lang</span>
                </div>
                <span className="text-[10px] font-mono text-slate-400 mt-0.5">
                  splang.shantopaul.com
                </span>
              </div>
            </Link>
            <Link href="/downloads">
              <Badge variant="success" className="hidden sm:inline-flex ml-2 cursor-pointer hover:border-cyan-400 transition-all">
                <span className="w-1.5 h-1.5 rounded-full bg-[#00D1FF] animate-ping" />
                v3.1.0 GA
              </Badge>
            </Link>
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
                    className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-sm font-medium text-slate-300 hover:text-white hover:bg-[#081735] hover:border-[#0A1F44] border border-transparent transition-all group"
                  >
                    <Icon className="w-4 h-4 text-slate-400 group-hover:text-cyan-400 transition-colors" />
                    <span>{link.name}</span>
                    <ExternalLink className="w-3 h-3 text-slate-500 group-hover:text-slate-300" />
                  </a>
                );
              }

              return (
                <Link
                  key={link.name}
                  href={link.href}
                  className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-sm font-medium transition-all ${
                    isActive
                      ? "text-cyan-300 bg-[#081735] border border-cyan-500/40 shadow-glow-sm"
                      : "text-slate-300 hover:text-white hover:bg-[#081735] hover:border-[#0A1F44] border border-transparent"
                  }`}
                >
                  <Icon className={`w-4 h-4 ${isActive ? "text-cyan-400" : "text-slate-400"}`} />
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
              className="flex items-center gap-2 px-3.5 py-1.5 rounded-xl border border-[#0A1F44] bg-[#081735]/80 text-slate-300 hover:text-white hover:border-cyan-500/40 hover:bg-[#0A1F44] transition-all text-sm font-medium shadow-sm"
            >
              <Github className="w-4 h-4 text-slate-400" />
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
              className="p-2 rounded-xl text-slate-400 hover:text-white hover:bg-[#081735] border border-transparent hover:border-[#0A1F44] focus:outline-none"
              aria-label="Toggle menu"
            >
              {mobileMenuOpen ? <X className="w-6 h-6" /> : <Menu className="w-6 h-6" />}
            </button>
          </div>
        </div>
      </div>

      {/* Mobile Drawer */}
      {mobileMenuOpen && (
        <div className="md:hidden border-b border-[#0A1F44] bg-[#020D21]/95 backdrop-blur-2xl px-4 pt-2 pb-6 space-y-2">
          {navLinks.map((link) => (
            <a
              key={link.name}
              href={link.href}
              target={link.external ? "_blank" : undefined}
              rel={link.external ? "noopener noreferrer" : undefined}
              onClick={() => setMobileMenuOpen(false)}
              className="flex items-center justify-between px-3.5 py-2.5 rounded-xl text-base font-medium text-slate-300 hover:text-white hover:bg-[#081735] border border-transparent hover:border-[#0A1F44]"
            >
              <span className="flex items-center gap-2.5">
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
              className="flex items-center justify-center gap-2 w-full py-2.5 rounded-xl border border-[#0A1F44] bg-[#081735] text-slate-200 text-sm font-medium hover:border-cyan-500/40"
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
