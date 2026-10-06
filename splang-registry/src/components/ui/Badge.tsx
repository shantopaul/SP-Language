import React from "react";
import { cn } from "@/lib/utils";

interface BadgeProps extends React.HTMLAttributes<HTMLSpanElement> {
  variant?: "default" | "success" | "cyan" | "outline" | "amber";
}

export const Badge: React.FC<BadgeProps> = ({
  className,
  variant = "default",
  children,
  ...props
}) => {
  const styles = {
    default: "bg-slate-800 text-slate-300 border-slate-700",
    success: "bg-emerald-950/80 text-emerald-400 border-emerald-800/80",
    cyan: "bg-cyan-950/80 text-cyan-400 border-cyan-800/80",
    amber: "bg-amber-950/80 text-amber-400 border-amber-800/80",
    outline: "bg-transparent text-slate-400 border-slate-800",
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
