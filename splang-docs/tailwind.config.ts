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
          50: "#edf5ff",
          100: "#d6e8ff",
          200: "#b5d7ff",
          300: "#85bfff",
          400: "#4da0ff",
          500: "#0057FF", // Official Primary Electric Blue
          600: "#0047d9",
          700: "#0038b3",
          800: "#002d8f",
          900: "#002778",
          950: "#00164a",
        },
        spbrand: {
          50: "#edf5ff",
          100: "#d6e8ff",
          500: "#0057FF",
          600: "#0047d9",
        },
        spcyan: {
          300: "#67e8f9",
          400: "#38e1ff",
          500: "#00D1FF", // Official Electric Cyan Accent
          600: "#00a8d6",
          700: "#0891b2",
        },
        spblue: {
          400: "#60a5fa",
          500: "#2563EB", // Official Cobalt UI Blue
          600: "#1d4ed8",
          700: "#1e40af",
        },
        canvas: {
          deep: "#020D21",     // Official Space Midnight Background
          surface: "#081735",  // Official Deep Navy Surface Card
          border: "#0A1F44",   // Official Structural Navy Border
          muted: "#777D89",    // Official Muted Slate Text
        },
      },
      fontFamily: {
        sans: ["var(--font-inter)", "-apple-system", "BlinkMacSystemFont", "Segoe UI", "Roboto", "sans-serif"],
        mono: ["var(--font-jetbrains-mono)", "ui-monospace", "SFMono-Regular", "Menlo", "Monaco", "Consolas", "monospace"],
      },
      boxShadow: {
        "glow-sm": "0 0 15px rgba(0, 209, 255, 0.15)",
        "glow-md": "0 0 25px rgba(0, 87, 255, 0.25)",
        "glow-lg": "0 0 45px rgba(0, 87, 255, 0.35)",
        "glow-cyan": "0 0 30px rgba(0, 209, 255, 0.25)",
      },
      animation: {
        "pulse-subtle": "pulse 3s cubic-bezier(0.4, 0, 0.6, 1) infinite",
        "pulse-glow": "pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite",
        "float": "float 4s ease-in-out infinite",
      },
      keyframes: {
        float: {
          "0%, 100%": { transform: "translateY(0)" },
          "50%": { transform: "translateY(-6px)" },
        },
      },
    },
  },
  plugins: [require("@tailwindcss/typography")],
};

export default config;
