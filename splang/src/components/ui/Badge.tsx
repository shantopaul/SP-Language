import React from "react";
import { cn } from "@/lib/utils";

export interface BadgeProps extends React.HTMLAttributes<HTMLSpanElement> {
  variant?: "default" | "success" | "info" | "warning" | "purple" | "outline";
}

export const Badge: React.FC<BadgeProps> = ({
  className,
  variant = "default",
  children,
  ...props
}) => {
  const variantStyles = {
    default: "bg-slate-800 text-slate-300 border-slate-700",
    success: "bg-emerald-950/60 text-emerald-400 border-emerald-800/60",
    info: "bg-cyan-950/60 text-cyan-400 border-cyan-800/60",
    warning: "bg-amber-950/60 text-amber-400 border-amber-800/60",
    purple: "bg-indigo-950/60 text-indigo-400 border-indigo-800/60",
    outline: "bg-transparent text-slate-400 border-slate-800",
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
