"use client";

import React, { useState, useEffect } from "react";
import { Sun, Moon, Laptop } from "lucide-react";
import { useTheme, Theme } from "./ThemeProvider";

interface ThemeToggleProps {
  variant?: "segmented" | "button";
  className?: string;
}

export const ThemeToggle: React.FC<ThemeToggleProps> = ({
  variant = "segmented",
  className = "",
}) => {
  const { theme, resolvedTheme, setTheme, toggleTheme } = useTheme();
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
  }, []);

  if (!mounted) {
    return (
      <div
        className={`inline-flex items-center p-1 rounded-xl border border-[#0A1F44] bg-[#081735]/70 h-8 w-[88px] ${className}`}
        aria-hidden="true"
      />
    );
  }

  if (variant === "button") {
    return (
      <button
        onClick={toggleTheme}
        className={`inline-flex items-center justify-center p-2 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/80 hover:bg-slate-100 dark:hover:bg-[#0A1F44] text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white transition-all shadow-sm ${className}`}
        title={`Current: ${theme} (${resolvedTheme}). Click to toggle mode.`}
        aria-label="Toggle dark/light theme"
      >
        {resolvedTheme === "dark" ? (
          <Moon className="w-4 h-4 text-[#00D1FF]" />
        ) : (
          <Sun className="w-4 h-4 text-amber-500" />
        )}
      </button>
    );
  }

  const options: { value: Theme; label: string; icon: React.ComponentType<{ className?: string }> }[] = [
    { value: "light", label: "Light mode", icon: Sun },
    { value: "dark", label: "Dark mode", icon: Moon },
    { value: "system", label: "System auto mode", icon: Laptop },
  ];

  return (
    <div
      role="group"
      aria-label="Theme mode selector"
      className={`inline-flex items-center p-0.5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-slate-100 dark:bg-[#081735]/80 backdrop-blur-md shadow-sm transition-all ${className}`}
    >
      {options.map((opt) => {
        const Icon = opt.icon;
        const isSelected = theme === opt.value;

        return (
          <button
            key={opt.value}
            onClick={() => setTheme(opt.value)}
            className={`relative p-1.5 rounded-lg text-xs transition-all duration-200 ${
              isSelected
                ? "bg-white dark:bg-[#0057FF] text-[#0057FF] dark:text-white shadow-sm font-semibold"
                : "text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200"
            }`}
            title={opt.label}
            aria-label={opt.label}
            aria-pressed={isSelected}
          >
            <Icon
              className={`w-3.5 h-3.5 ${
                isSelected
                  ? opt.value === "light"
                    ? "text-amber-500"
                    : opt.value === "dark"
                    ? "text-[#00D1FF] dark:text-white"
                    : "text-[#0057FF] dark:text-white"
                  : ""
              }`}
            />
          </button>
        );
      })}
    </div>
  );
};
