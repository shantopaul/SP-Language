import React from "react";
import { cn } from "@/lib/utils";

export const Card: React.FC<React.HTMLAttributes<HTMLDivElement>> = ({
  className,
  children,
  ...props
}) => {
  return (
    <div
      className={cn(
        "rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 backdrop-blur-xl p-6 shadow-sm dark:shadow-xl transition-all duration-300 hover:border-blue-400 dark:hover:border-cyan-500/40 hover:shadow-md dark:hover:shadow-[0_8px_30px_rgba(0,87,255,0.12)]",
        className
      )}
      {...props}
    >
      {children}
    </div>
  );
};

export const CardHeader: React.FC<React.HTMLAttributes<HTMLDivElement>> = ({
  className,
  children,
  ...props
}) => {
  return (
    <div className={cn("flex flex-col space-y-1.5 mb-4", className)} {...props}>
      {children}
    </div>
  );
};

export const CardTitle: React.FC<React.HTMLAttributes<HTMLHeadingElement>> = ({
  className,
  children,
  ...props
}) => {
  return (
    <h3
      className={cn("text-lg font-bold text-slate-900 dark:text-white tracking-tight", className)}
      {...props}
    >
      {children}
    </h3>
  );
};

export const CardDescription: React.FC<React.HTMLAttributes<HTMLParagraphElement>> = ({
  className,
  children,
  ...props
}) => {
  return (
    <p className={cn("text-sm text-slate-600 dark:text-slate-300/90 leading-relaxed", className)} {...props}>
      {children}
    </p>
  );
};

export const CardContent: React.FC<React.HTMLAttributes<HTMLDivElement>> = ({
  className,
  children,
  ...props
}) => {
  return <div className={cn("", className)} {...props}>{children}</div>;
};
