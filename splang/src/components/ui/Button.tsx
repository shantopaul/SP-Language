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
      "inline-flex items-center justify-center font-medium transition-all duration-200 rounded-lg focus:outline-none focus:ring-2 focus:ring-offset-2 disabled:opacity-50 disabled:pointer-events-none";

    const variantStyles = {
      primary:
        "bg-gradient-to-r from-emerald-500 via-teal-500 to-cyan-500 text-slate-950 font-semibold shadow-lg shadow-emerald-500/20 hover:shadow-emerald-500/35 hover:brightness-110 focus:ring-emerald-400 active:scale-[0.98]",
      secondary:
        "bg-slate-800 text-slate-100 hover:bg-slate-700 border border-slate-700/80 shadow-sm focus:ring-slate-500 active:scale-[0.98]",
      outline:
        "border border-slate-700 text-slate-200 hover:bg-slate-800/60 hover:border-slate-600 focus:ring-slate-500 active:scale-[0.98]",
      ghost:
        "text-slate-300 hover:text-white hover:bg-slate-800/50 focus:ring-slate-500",
      glow:
        "bg-cyan-500/10 text-cyan-400 border border-cyan-500/30 hover:bg-cyan-500/20 hover:border-cyan-400/50 shadow-[0_0_20px_rgba(6,182,212,0.15)] focus:ring-cyan-400",
    };

    const sizeStyles = {
      sm: "text-xs px-3 py-1.5 gap-1.5",
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
