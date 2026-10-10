import React from "react";
import { cn } from "@/lib/utils";

export interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: "primary" | "secondary" | "outline" | "ghost" | "glow";
  size?: "sm" | "md" | "lg";
  asChild?: boolean;
}

export const Button = React.forwardRef<HTMLButtonElement, ButtonProps>(
  ({ className, variant = "primary", size = "md", children, ...props }, ref) => {
    const baseStyles =
      "inline-flex items-center justify-center font-medium transition-all duration-200 rounded-xl focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-[#020D21] disabled:opacity-50 disabled:pointer-events-none";

    const variantStyles = {
      primary:
        "bg-gradient-to-r from-[#0057FF] via-[#0047d9] to-[#00D1FF] text-white font-semibold shadow-lg shadow-blue-600/25 hover:shadow-cyan-500/35 hover:brightness-110 focus:ring-cyan-400 active:scale-[0.98]",
      secondary:
        "bg-[#081735] text-slate-100 hover:bg-[#0A1F44] hover:text-white border border-[#0A1F44] hover:border-cyan-500/40 shadow-sm focus:ring-blue-500 active:scale-[0.98]",
      outline:
        "border border-[#0A1F44] bg-[#020D21]/60 text-slate-200 hover:bg-[#081735] hover:border-cyan-500/40 hover:text-white focus:ring-cyan-500 active:scale-[0.98]",
      ghost:
        "text-slate-300 hover:text-white hover:bg-[#081735]/60 focus:ring-cyan-500",
      glow:
        "bg-cyan-500/10 text-cyan-300 border border-cyan-500/30 hover:bg-cyan-500/20 hover:border-cyan-400 shadow-glow-sm focus:ring-cyan-400",
    };

    const sizeStyles = {
      sm: "text-xs px-3.5 py-1.5 gap-1.5",
      md: "text-sm px-4 py-2 gap-2",
      lg: "text-base px-6 py-3 gap-2.5",
    };

    return (
      <button
        ref={ref}
        className={cn(baseStyles, variantStyles[variant], sizeStyles[size], className)}
        {...props}
      >
        {children}
      </button>
    );
  }
);

Button.displayName = "Button";
