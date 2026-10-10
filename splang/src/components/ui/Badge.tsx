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
    default: "bg-slate-100 dark:bg-[#081735] text-slate-700 dark:text-slate-300 border-slate-200 dark:border-[#0A1F44]",
    secondary: "bg-slate-100 dark:bg-[#081735] text-slate-700 dark:text-slate-300 border-slate-200 dark:border-[#0A1F44]",
    success: "bg-blue-50 dark:bg-cyan-950/70 text-[#0057FF] dark:text-cyan-300 border-blue-200 dark:border-cyan-700/60 shadow-sm dark:shadow-[0_0_12px_rgba(0,209,255,0.15)]",
    info: "bg-sky-50 dark:bg-blue-950/70 text-sky-700 dark:text-cyan-300 border-sky-200 dark:border-blue-800/60",
    primary: "bg-blue-50 dark:bg-blue-900/60 text-blue-700 dark:text-blue-200 border-blue-200 dark:border-blue-700/70",
    warning: "bg-amber-50 dark:bg-amber-950/60 text-amber-800 dark:text-amber-300 border-amber-200 dark:border-amber-800/60",
    purple: "bg-purple-50 dark:bg-indigo-950/60 text-purple-700 dark:text-indigo-300 border-purple-200 dark:border-indigo-800/60",
    outline: "bg-transparent text-slate-600 dark:text-slate-400 border-slate-300 dark:border-[#0A1F44]",
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
