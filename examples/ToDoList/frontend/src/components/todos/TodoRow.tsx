"use client";

import React from "react";
import { Calendar, Check, Pencil, Trash2 } from "lucide-react";
import { PriorityBadge, StatusBadge } from "@/components/ui/Badge";
import { cn } from "@/lib/cn";
import { formatDueLabel, isDueToday, isOverdue } from "@/lib/date";
import type { Todo } from "@/types/todo";

interface TodoRowProps {
  todo: Todo;
  selected: boolean;
  onToggleSelect: (id: string) => void;
  onToggleComplete: (todo: Todo) => Promise<void>;
  onOpenDetail: (todo: Todo) => void;
  onDelete: (id: string) => Promise<boolean>;
}

export function TodoRow({
  todo,
  selected,
  onToggleSelect,
  onToggleComplete,
  onOpenDetail,
  onDelete,
}: TodoRowProps) {
  const isDone = todo.status === "done";
  const overdue = isOverdue(todo.due_date, todo.status);
  const dueToday = isDueToday(todo.due_date);

  return (
    <div
      className={cn(
        "group flex items-start justify-between gap-3 rounded-xl border bg-white p-3.5 transition-all hover:shadow-soft sm:items-center dark:bg-slate-900",
        selected
          ? "border-blue-400 ring-1 ring-blue-500/30 dark:border-blue-700"
          : "border-slate-200/90 hover:border-slate-300 dark:border-slate-800 dark:hover:border-slate-700"
      )}
    >
      <div className="flex min-w-0 flex-1 items-start gap-3 sm:items-center">
        <input
          type="checkbox"
          checked={selected}
          onChange={() => onToggleSelect(todo.id)}
          aria-label={`Select task ${todo.title}`}
          className="mt-1 h-4 w-4 shrink-0 rounded border-slate-300 text-blue-600 focus:ring-blue-500 sm:mt-0 dark:border-slate-600 dark:bg-slate-800"
        />

        <button
          type="button"
          onClick={() => void onToggleComplete(todo)}
          aria-label={isDone ? `Reopen ${todo.title}` : `Complete ${todo.title}`}
          className={cn(
            "mt-0.5 flex h-5 w-5 shrink-0 items-center justify-center rounded-full border transition-colors sm:mt-0",
            isDone
              ? "border-emerald-500 bg-emerald-500 text-white"
              : "border-slate-300 text-transparent hover:border-blue-500 dark:border-slate-600 dark:hover:border-blue-400"
          )}
        >
          <Check className="h-3 w-3 stroke-[3]" />
        </button>

        <button
          type="button"
          onClick={() => onOpenDetail(todo)}
          className="min-w-0 flex-1 text-left focus:outline-none"
        >
          <p
            className={cn(
              "truncate text-sm font-semibold transition-colors",
              isDone
                ? "text-slate-400 line-through dark:text-slate-500"
                : "text-slate-900 group-hover:text-blue-600 dark:text-slate-100 dark:group-hover:text-blue-400"
            )}
          >
            {todo.title}
          </p>
          <div className="mt-1 flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400">
            {todo.category && (
              <span className="inline-flex items-center gap-1.5 font-medium text-slate-600 dark:text-slate-300">
                <span
                  className="h-2 w-2 rounded-full"
                  style={{ backgroundColor: todo.category.color }}
                />
                {todo.category.name}
              </span>
            )}
            {todo.due_date && (
              <span
                className={cn(
                  "inline-flex items-center gap-1 rounded-md px-1.5 py-0.5 font-mono text-[11px]",
                  overdue
                    ? "bg-rose-50 font-semibold text-rose-700 dark:bg-rose-950/70 dark:text-rose-300"
                    : dueToday && !isDone
                      ? "bg-blue-50 font-semibold text-blue-700 dark:bg-blue-950/70 dark:text-blue-300"
                      : "text-slate-500 dark:text-slate-400"
                )}
              >
                <Calendar className="h-3 w-3" />
                {formatDueLabel(todo.due_date)}
                {overdue ? " (Overdue)" : ""}
              </span>
            )}
            {todo.description && (
              <span className="hidden max-w-xs truncate text-slate-400 md:inline">
                · {todo.description}
              </span>
            )}
          </div>
        </button>
      </div>

      <div className="flex shrink-0 items-center gap-2">
        <div className="hidden items-center gap-1.5 sm:flex">
          <PriorityBadge priority={todo.priority} />
          <StatusBadge status={todo.status} />
        </div>

        <div className="flex items-center gap-0.5 opacity-100 transition-opacity sm:opacity-0 sm:group-hover:opacity-100 sm:focus-within:opacity-100">
          <button
            type="button"
            onClick={() => onOpenDetail(todo)}
            aria-label={`Edit task ${todo.title}`}
            className="rounded-lg p-1.5 text-slate-400 transition-colors hover:bg-slate-100 hover:text-slate-700 dark:hover:bg-slate-800 dark:hover:text-slate-200"
          >
            <Pencil className="h-3.5 w-3.5" />
          </button>
          <button
            type="button"
            onClick={() => void onDelete(todo.id)}
            aria-label={`Delete task ${todo.title}`}
            className="rounded-lg p-1.5 text-slate-400 transition-colors hover:bg-rose-50 hover:text-rose-600 dark:hover:bg-rose-950/50 dark:hover:text-rose-400"
          >
            <Trash2 className="h-3.5 w-3.5" />
          </button>
        </div>
      </div>
    </div>
  );
}
