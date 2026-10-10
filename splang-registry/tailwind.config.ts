import type { Config } from "tailwindcss";

const config: Config = {
  darkMode: "class",
  content: [
    "./src/pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./src/components/**/*.{js,ts,jsx,tsx,mdx}",
    "./src/app/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      colors: {
        brand: {
          primary: "#0057FF",
          accent: "#00D1FF",
          secondary: "#2563EB",
        },
        canvas: {
          deep: "#020D21",
          surface: "#081735",
          border: "#0A1F44",
        },
        spbrand: {
          500: "#0057FF",
          600: "#2563EB",
        },
        spcyan: {
          400: "#00D1FF",
          500: "#00B4D8",
          600: "#0077B6",
        },
      },
      fontFamily: {
        sans: ["var(--font-inter)", "-apple-system", "BlinkMacSystemFont", "Segoe UI", "Roboto", "sans-serif"],
        mono: ["var(--font-jetbrains-mono)", "ui-monospace", "SFMono-Regular", "Menlo", "Monaco", "Consolas", "monospace"],
      },
    },
  },
  plugins: [],
};

export default config;
