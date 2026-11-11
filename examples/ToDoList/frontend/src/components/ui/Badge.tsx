import React from "react";
import { cn } from "@/lib/cn";
import type { TodoPriority, TodoStatus } from "@/types/todo";

export function Badge({
  children,
  className,
}: {
  children: React.ReactNode;
  className?: string;
}) {
  return (
    <span
      className={cn(
        "inline-flex items-center gap-1.5 rounded-full px-2.5 py-0.5 text-xs font-medium",
        className
      )}
    >
      {children}
    </span>
  );
}

const STATUS_STYLES: Record<TodoStatus, { label: string; classes: string }> = {
  todo: {
    label: "To Do",
    classes:
      "bg-slate-100 text-slate-700 dark:bg-slate-800 dark:text-slate-300",
  },
  in_progress: {
    label: "In Progress",
    classes:
      "bg-blue-50 text-blue-700 ring-1 ring-inset ring-blue-600/20 dark:bg-blue-950/60 dark:text-blue-300 dark:ring-blue-500/30",
  },
  done: {
    label: "Done",
    classes:
      "bg-emerald-50 text-emerald-700 ring-1 ring-inset ring-emerald-600/20 dark:bg-emerald-950/60 dark:text-emerald-300 dark:ring-emerald-500/30",
  },
  archived: {
    label: "Archived",
    classes:
      "bg-zinc-100 text-zinc-600 dark:bg-zinc-800 dark:text-zinc-400",
  },
};

export function StatusBadge({ status }: { status: TodoStatus }) {
  const cfg = STATUS_STYLES[status] ?? STATUS_STYLES.todo;
  return <Badge className={cfg.classes}>{cfg.label}</Badge>;
}

const PRIORITY_STYLES: Record<
  TodoPriority,
  { label: string; dot: string; classes: string }
> = {
  low: {
    label: "Low",
    dot: "bg-slate-400",
    classes:
      "bg-slate-100 text-slate-600 dark:bg-slate-800/80 dark:text-slate-300",
  },
  medium: {
    label: "Medium",
    dot: "bg-sky-500",
    classes:
      "bg-sky-50 text-sky-700 dark:bg-sky-950/60 dark:text-sky-300",
  },
  high: {
    label: "High",
    dot: "bg-amber-500",
    classes:
      "bg-amber-50 text-amber-700 dark:bg-amber-950/60 dark:text-amber-300",
  },
  urgent: {
    label: "Urgent",
    dot: "bg-rose-500",
    classes:
      "bg-rose-50 text-rose-700 ring-1 ring-inset ring-rose-600/20 dark:bg-rose-950/60 dark:text-rose-300 dark:ring-rose-500/30",
  },
};

export function PriorityBadge({ priority }: { priority: TodoPriority }) {
  const cfg = PRIORITY_STYLES[priority] ?? PRIORITY_STYLES.medium;
  return (
    <Badge className={cfg.classes}>
      <span className={cn("h-1.5 w-1.5 rounded-full", cfg.dot)} />
      {cfg.label}
    </Badge>
  );
}
