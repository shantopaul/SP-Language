import React from "react";
import { Info, Lightbulb, AlertTriangle, AlertCircle, ShieldAlert } from "lucide-react";
import { cn } from "@/lib/utils";

interface CalloutProps {
  type?: "note" | "tip" | "important" | "warning" | "caution";
  title?: string;
  children: React.ReactNode;
  className?: string;
}

interface CalloutConfig {
  icon: React.ComponentType<{ className?: string }>;
  defaultTitle: string;
  border: string;
  bg: string;
  text: string;
  iconColor: string;
  glow?: string;
}

export const Callout: React.FC<CalloutProps> = ({
  type = "note",
  title,
  children,
  className,
}) => {
  const configs: Record<NonNullable<CalloutProps["type"]>, CalloutConfig> = {
    note: {
      icon: Info,
      defaultTitle: "Note",
      border: "border-[#0057FF]/40",
      bg: "bg-[#081735]/80",
      text: "text-blue-300",
      iconColor: "text-[#00D1FF]",
    },
    tip: {
      icon: Lightbulb,
      defaultTitle: "Tip",
      border: "border-[#00D1FF]/40",
      bg: "bg-[#081735]/80",
      text: "text-[#00D1FF]",
      iconColor: "text-[#00D1FF]",
      glow: "shadow-[0_0_20px_rgba(0,209,255,0.1)]",
    },
    important: {
      icon: AlertCircle,
      defaultTitle: "Important",
      border: "border-indigo-500/40",
      bg: "bg-[#081735]/80",
      text: "text-indigo-300",
      iconColor: "text-indigo-400",
    },
    warning: {
      icon: AlertTriangle,
      defaultTitle: "Warning",
      border: "border-amber-500/40",
      bg: "bg-[#081735]/80",
      text: "text-amber-300",
      iconColor: "text-amber-400",
    },
    caution: {
      icon: ShieldAlert,
      defaultTitle: "Caution",
      border: "border-rose-500/40",
      bg: "bg-[#081735]/80",
      text: "text-rose-300",
      iconColor: "text-rose-400",
    },
  };

  const config = configs[type];
  const Icon = config.icon;

  return (
    <div
      className={cn(
        "rounded-xl border p-4 my-5 flex items-start gap-3 backdrop-blur-md transition-all",
        config.border,
        config.bg,
        config.glow,
        className
      )}
    >
      <Icon className={cn("w-5 h-5 shrink-0 mt-0.5", config.iconColor)} />
      <div className="flex-1 text-xs sm:text-sm">
        <h5 className={cn("font-bold mb-1 tracking-tight", config.text)}>
          {title || config.defaultTitle}
        </h5>
        <div className="text-slate-300 leading-relaxed [&>p]:mb-2 [&>p:last-child]:mb-0">
          {children}
        </div>
      </div>
    </div>
  );
};
