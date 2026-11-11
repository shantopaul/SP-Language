import React from "react";
import { AlertTriangle, CalendarClock, CheckCircle2, Layers } from "lucide-react";
import { cn } from "@/lib/cn";
import type { TodoQuery, TodoStats } from "@/types/todo";

interface StatsCardsProps {
  stats: TodoStats;
  query: TodoQuery;
  onQuickFilter: (patch: Partial<TodoQuery>) => void;
}

export function StatsCards({ stats, query, onQuickFilter }: StatsCardsProps) {
  const completionPct =
    stats.total > 0 ? Math.round((stats.done / stats.total) * 100) : 0;

  return (
    <div className="grid grid-cols-2 gap-3 sm:grid-cols-4 sm:gap-4">
      <StatCard
        label="Total Tasks"
        value={stats.total}
        sublabel={`${completionPct}% completed`}
        icon={<Layers className="h-4 w-4 text-slate-600 dark:text-slate-300" />}
        active={
          (query.status ?? "all") === "all" &&
          (query.due ?? "all") === "all"
        }
        onClick={() => onQuickFilter({ status: "all", due: "all", page: 1 })}
      />
      <StatCard
        label="In Progress"
        value={stats.in_progress}
        sublabel={`${stats.todo} queued`}
        icon={<CheckCircle2 className="h-4 w-4 text-blue-600 dark:text-blue-400" />}
        active={query.status === "in_progress" && (query.due ?? "all") === "all"}
        onClick={() =>
          onQuickFilter({ status: "in_progress", due: "all", page: 1 })
        }
      />
      <StatCard
        label="Due Today"
        value={stats.due_today}
        sublabel={`${stats.urgent} urgent open`}
        icon={<CalendarClock className="h-4 w-4 text-amber-600 dark:text-amber-400" />}
        active={query.due === "today"}
        onClick={() => onQuickFilter({ status: "all", due: "today", page: 1 })}
      />
      <StatCard
        label="Overdue"
        value={stats.overdue}
        sublabel={stats.overdue > 0 ? "Needs attention" : "On schedule"}
        icon={
          <AlertTriangle
            className={cn(
              "h-4 w-4",
              stats.overdue > 0
                ? "text-rose-600 dark:text-rose-400"
                : "text-emerald-600 dark:text-emerald-400"
            )}
          />
        }
        danger={stats.overdue > 0}
        active={query.due === "overdue"}
        onClick={() =>
          onQuickFilter({ status: "all", due: "overdue", page: 1 })
        }
      />
    </div>
  );
}

function StatCard({
  label,
  value,
  sublabel,
  icon,
  active,
  danger = false,
  onClick,
}: {
  label: string;
  value: number;
  sublabel: string;
  icon: React.ReactNode;
  active?: boolean;
  danger?: boolean;
  onClick: () => void;
}) {
  return (
    <button
      type="button"
      onClick={onClick}
      className={cn(
        "flex flex-col rounded-xl border p-4 text-left transition-all hover:shadow-soft",
        active
          ? "border-blue-300 bg-blue-50/40 ring-1 ring-blue-500/20 dark:border-blue-800 dark:bg-blue-950/30"
          : "border-slate-200 bg-white hover:border-slate-300 dark:border-slate-800 dark:bg-slate-900 dark:hover:border-slate-700"
      )}
    >
      <div className="flex w-full items-center justify-between">
        <span className="text-xs font-medium text-slate-500 dark:text-slate-400">
          {label}
        </span>
        {icon}
      </div>
      <span
        className={cn(
          "mt-2 font-mono text-2xl font-bold tracking-tight",
          danger
            ? "text-rose-600 dark:text-rose-400"
            : "text-slate-900 dark:text-white"
        )}
      >
        {value}
      </span>
      <span className="mt-0.5 text-[11px] text-slate-400">{sublabel}</span>
    </button>
  );
}
