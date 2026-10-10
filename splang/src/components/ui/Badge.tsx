import React from "react";
import { cn } from "@/lib/utils";

export interface BadgeProps extends React.HTMLAttributes<HTMLSpanElement> {
  variant?: "default" | "secondary" | "success" | "info" | "primary" | "warning" | "purple" | "outline";
}

export const Badge: React.FC<BadgeProps> = ({
  className,
  variant = "default",
  children,
  ...props
}) => {
  const variantStyles = {
    default: "bg-[#081735] text-slate-300 border-[#0A1F44]",
    secondary: "bg-[#081735] text-slate-300 border-[#0A1F44]",
    success: "bg-cyan-950/70 text-cyan-300 border-cyan-700/60 shadow-[0_0_12px_rgba(0,209,255,0.15)]",
    info: "bg-blue-950/70 text-cyan-300 border-blue-800/60",
    primary: "bg-blue-900/60 text-blue-200 border-blue-700/70",
    warning: "bg-amber-950/60 text-amber-300 border-amber-800/60",
    purple: "bg-indigo-950/60 text-indigo-300 border-indigo-800/60",
    outline: "bg-transparent text-slate-400 border-[#0A1F44]",
  };

  return (
    <span
      className={cn(
        "inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full text-xs font-medium border transition-colors",
        variantStyles[variant],
        className
      )}
      {...props}
    >
      {children}
    </span>
  );
};
