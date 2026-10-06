import React from "react";
import { Info, Lightbulb, AlertTriangle, AlertCircle, ShieldAlert } from "lucide-react";
import { cn } from "@/lib/utils";

interface CalloutProps {
  type?: "note" | "tip" | "important" | "warning" | "caution";
  title?: string;
  children: React.ReactNode;
  className?: string;
}

export const Callout: React.FC<CalloutProps> = ({
  type = "note",
  title,
  children,
  className,
}) => {
  const configs = {
    note: {
      icon: Info,
      defaultTitle: "Note",
      border: "border-blue-500/40",
      bg: "bg-blue-950/20",
      text: "text-blue-300",
      iconColor: "text-blue-400",
    },
    tip: {
      icon: Lightbulb,
      defaultTitle: "Tip",
      border: "border-emerald-500/40",
      bg: "bg-emerald-950/20",
      text: "text-emerald-300",
      iconColor: "text-emerald-400",
    },
    important: {
      icon: AlertCircle,
      defaultTitle: "Important",
      border: "border-cyan-500/40",
      bg: "bg-cyan-950/20",
      text: "text-cyan-300",
      iconColor: "text-cyan-400",
    },
    warning: {
      icon: AlertTriangle,
      defaultTitle: "Warning",
      border: "border-amber-500/40",
      bg: "bg-amber-950/20",
      text: "text-amber-300",
      iconColor: "text-amber-400",
    },
    caution: {
      icon: ShieldAlert,
      defaultTitle: "Caution",
      border: "border-rose-500/40",
      bg: "bg-rose-950/20",
      text: "text-rose-300",
      iconColor: "text-rose-400",
    },
  };

  const config = configs[type];
  const Icon = config.icon;

  return (
    <div
      className={cn(
        "rounded-xl border p-4 my-4 flex items-start gap-3 backdrop-blur-sm",
        config.border,
        config.bg,
        className
      )}
    >
      <Icon className={cn("w-5 h-5 shrink-0 mt-0.5", config.iconColor)} />
      <div className="flex-1 text-xs sm:text-sm">
        <h5 className={cn("font-semibold mb-1", config.text)}>
          {title || config.defaultTitle}
        </h5>
        <div className="text-slate-300 leading-relaxed [&>p]:mb-2 [&>p:last-child]:mb-0">
          {children}
        </div>
      </div>
    </div>
  );
};
