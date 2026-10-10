"use client";

import React, { createContext, useContext, useEffect, useState } from "react";

export type Theme = "light" | "dark" | "system";

interface ThemeContextType {
  theme: Theme;
  resolvedTheme: "light" | "dark";
  setTheme: (theme: Theme) => void;
  toggleTheme: () => void;
}

const ThemeContext = createContext<ThemeContextType | undefined>(undefined);

export function ThemeProvider({ children }: { children: React.ReactNode }) {
  const [theme, setThemeState] = useState<Theme>("system");
  const [resolvedTheme, setResolvedTheme] = useState<"light" | "dark">("dark");
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    const saved = (localStorage.getItem("sp_theme") as Theme) || "system";
    setThemeState(saved);

    const mediaQuery = window.matchMedia("(prefers-color-scheme: dark)");

    const applyTheme = (currentTheme: Theme) => {
      const isDark =
        currentTheme === "dark" ||
        ((currentTheme === "system" || !currentTheme) && mediaQuery.matches);

      const root = document.documentElement;
      if (isDark) {
        root.classList.add("dark");
        root.classList.remove("light");
        setResolvedTheme("dark");
      } else {
        root.classList.remove("dark");
        root.classList.add("light");
        setResolvedTheme("light");
      }
    };

    applyTheme(saved);
    setMounted(true);

    const handleSystemChange = (e: MediaQueryListEvent) => {
      const currentSaved = (localStorage.getItem("sp_theme") as Theme) || "system";
      if (currentSaved === "system") {
        const root = document.documentElement;
        if (e.matches) {
          root.classList.add("dark");
          root.classList.remove("light");
          setResolvedTheme("dark");
        } else {
          root.classList.remove("dark");
          root.classList.add("light");
          setResolvedTheme("light");
        }
      }
    };

    mediaQuery.addEventListener("change", handleSystemChange);
    return () => mediaQuery.removeEventListener("change", handleSystemChange);
  }, []);

  const setTheme = (newTheme: Theme) => {
    setThemeState(newTheme);
    try {
      localStorage.setItem("sp_theme", newTheme);
    } catch (e) {}

    const mediaQuery = window.matchMedia("(prefers-color-scheme: dark)");
    const isDark =
      newTheme === "dark" ||
      ((newTheme === "system" || !newTheme) && mediaQuery.matches);

    const root = document.documentElement;
    if (isDark) {
      root.classList.add("dark");
      root.classList.remove("light");
      setResolvedTheme("dark");
    } else {
      root.classList.remove("dark");
      root.classList.add("light");
      setResolvedTheme("light");
    }
  };

  const toggleTheme = () => {
    if (resolvedTheme === "dark") {
      setTheme("light");
    } else {
      setTheme("dark");
    }
  };

  return (
    <ThemeContext.Provider value={{ theme, resolvedTheme, setTheme, toggleTheme }}>
      {children}
    </ThemeContext.Provider>
  );
}

export function useTheme() {
  const context = useContext(ThemeContext);
  if (!context) {
    return {
      theme: "system" as Theme,
      resolvedTheme: "dark" as "light" | "dark",
      setTheme: () => {},
      toggleTheme: () => {},
    };
  }
  return context;
}
