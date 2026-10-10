import React from "react";
import { cn } from "@/lib/utils";

interface BadgeProps extends React.HTMLAttributes<HTMLSpanElement> {
  variant?: "default" | "success" | "cyan" | "primary" | "outline" | "amber";
}

export const Badge: React.FC<BadgeProps> = ({
  className,
  variant = "default",
  children,
  ...props
}) => {
  const styles = {
    default: "bg-slate-100 dark:bg-[#081735] text-slate-700 dark:text-slate-300 border-slate-200 dark:border-[#0A1F44]",
    success: "bg-emerald-50 dark:bg-emerald-950/40 text-emerald-700 dark:text-emerald-400 border-emerald-200 dark:border-emerald-800/40",
    cyan: "bg-blue-50 dark:bg-[#0057FF]/15 text-[#0057FF] dark:text-[#00D1FF] border-blue-200 dark:border-[#00D1FF]/40 shadow-sm dark:shadow-[0_0_10px_rgba(0,209,255,0.15)]",
    primary: "bg-blue-50 dark:bg-[#0057FF]/20 text-[#0057FF] dark:text-[#60A5FA] border-blue-200 dark:border-[#0057FF]/40",
    amber: "bg-amber-50 dark:bg-amber-950/80 text-amber-700 dark:text-amber-400 border-amber-200 dark:border-amber-800/80",
    outline: "bg-transparent text-slate-600 dark:text-slate-400 border-slate-300 dark:border-[#0A1F44]",
  };

  return (
    <span
      className={cn(
        "inline-flex items-center gap-1 px-2.5 py-0.5 rounded-full text-xs font-medium border font-mono",
        styles[variant],
        className
      )}
      {...props}
    >
      {children}
    </span>
  );
};
