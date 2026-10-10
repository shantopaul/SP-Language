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
    default: "bg-[#081735] text-slate-300 border-[#0A1F44]",
    success: "bg-[#0057FF]/20 text-[#00D1FF] border-[#00D1FF]/40",
    cyan: "bg-[#0057FF]/15 text-[#00D1FF] border-[#00D1FF]/40 shadow-[0_0_10px_rgba(0,209,255,0.15)]",
    primary: "bg-[#0057FF]/20 text-[#60A5FA] border-[#0057FF]/40",
    amber: "bg-amber-950/80 text-amber-400 border-amber-800/80",
    outline: "bg-transparent text-slate-400 border-[#0A1F44]",
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
