const TOKEN_KEY = "todolist_access_token";
const THEME_KEY = "todolist_theme";

export type ThemeMode = "light" | "dark";

export const tokenStorage = {
  get(): string | null {
    if (typeof window === "undefined") {
      return null;
    }
    try {
      return window.localStorage.getItem(TOKEN_KEY);
    } catch {
      return null;
    }
  },

  set(token: string): void {
    if (typeof window === "undefined") {
      return;
    }
    try {
      window.localStorage.setItem(TOKEN_KEY, token);
    } catch {
      // Ignore storage quota / privacy mode failures
    }
  },

  clear(): void {
    if (typeof window === "undefined") {
      return;
    }
    try {
      window.localStorage.removeItem(TOKEN_KEY);
    } catch {
      // Ignore storage failures
    }
  },
};

export const themeStorage = {
  get(): ThemeMode {
    if (typeof window === "undefined") {
      return "light";
    }
    try {
      const saved = window.localStorage.getItem(THEME_KEY);
      if (saved === "dark" || saved === "light") {
        return saved;
      }
      return window.matchMedia("(prefers-color-scheme: dark)").matches
        ? "dark"
        : "light";
    } catch {
      return "light";
    }
  },

  set(mode: ThemeMode): void {
    if (typeof window === "undefined") {
      return;
    }
    try {
      window.localStorage.setItem(THEME_KEY, mode);
    } catch {
      // Ignore storage failures
    }
  },
};
