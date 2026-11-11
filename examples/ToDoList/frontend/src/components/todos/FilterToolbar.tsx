"use client";

import React from "react";
import { ArrowDownUp, RotateCcw, Search, X } from "lucide-react";
import type {
  SortOrder,
  TodoPriority,
  TodoQuery,
  TodoSortField,
  TodoStatus,
} from "@/types/todo";

interface FilterToolbarProps {
  query: TodoQuery;
  searchValue: string;
  totalVisible: number;
  allVisibleSelected: boolean;
  onToggleSelectAll: () => void;
  onSearchChange: (value: string) => void;
  onUpdateFilters: (patch: Partial<TodoQuery>) => void;
  onResetFilters: () => void;
}

export function FilterToolbar({
  query,
  searchValue,
  totalVisible,
  allVisibleSelected,
  onToggleSelectAll,
  onSearchChange,
  onUpdateFilters,
  onResetFilters,
}: FilterToolbarProps) {
  const hasActiveFilters =
    (query.status ?? "all") !== "all" ||
    (query.priority ?? "all") !== "all" ||
    (query.category_id ?? "all") !== "all" ||
    (query.due ?? "all") !== "all" ||
    (query.q ?? "").trim().length > 0;

  return (
    <div className="flex flex-col gap-3 rounded-xl border border-slate-200/80 bg-white px-3.5 py-2.5 sm:flex-row sm:items-center sm:justify-between dark:border-slate-800 dark:bg-slate-900">
      <div className="flex flex-wrap items-center gap-2.5">
        <label className="inline-flex cursor-pointer items-center gap-2 text-xs font-medium text-slate-600 dark:text-slate-400">
          <input
            type="checkbox"
            checked={totalVisible > 0 && allVisibleSelected}
            disabled={totalVisible === 0}
            onChange={onToggleSelectAll}
            className="h-4 w-4 rounded border-slate-300 text-blue-600 focus:ring-blue-500 dark:border-slate-600 dark:bg-slate-800"
          />
          Select all
        </label>

        <div className="relative flex-1 sm:hidden">
          <Search className="pointer-events-none absolute left-2.5 top-1/2 h-3.5 w-3.5 -translate-y-1/2 text-slate-400" />
          <input
            type="text"
            value={searchValue}
            onChange={(e) => onSearchChange(e.target.value)}
            placeholder="Search tasks..."
            className="h-8 w-full rounded-lg border border-slate-200 bg-slate-50 pl-8 pr-7 text-xs dark:border-slate-700 dark:bg-slate-800"
          />
          {searchValue && (
            <button
              type="button"
              onClick={() => onSearchChange("")}
              className="absolute right-2 top-1/2 -translate-y-1/2 text-slate-400"
            >
              <X className="h-3 w-3" />
            </button>
          )}
        </div>

        <select
          value={query.status ?? "all"}
          onChange={(e) =>
            onUpdateFilters({
              status: e.target.value as "all" | "active" | TodoStatus,
            })
          }
          aria-label="Filter by status"
          className="h-8 rounded-lg border border-slate-200 bg-slate-50 px-2.5 text-xs font-medium text-slate-700 focus:border-blue-500 focus:outline-none dark:border-slate-700 dark:bg-slate-800 dark:text-slate-200"
        >
          <option value="all">All Statuses</option>
          <option value="active">Active (To Do + Progress)</option>
          <option value="todo">To Do</option>
          <option value="in_progress">In Progress</option>
          <option value="done">Done</option>
          <option value="archived">Archived</option>
        </select>

        <select
          value={query.priority ?? "all"}
          onChange={(e) =>
            onUpdateFilters({
              priority: e.target.value as "all" | TodoPriority,
            })
          }
          aria-label="Filter by priority"
          className="h-8 rounded-lg border border-slate-200 bg-slate-50 px-2.5 text-xs font-medium text-slate-700 focus:border-blue-500 focus:outline-none dark:border-slate-700 dark:bg-slate-800 dark:text-slate-200"
        >
          <option value="all">All Priorities</option>
          <option value="urgent">Urgent</option>
          <option value="high">High</option>
          <option value="medium">Medium</option>
          <option value="low">Low</option>
        </select>

        {hasActiveFilters && (
          <button
            type="button"
            onClick={onResetFilters}
            className="inline-flex h-8 items-center gap-1 rounded-lg px-2.5 text-xs font-medium text-slate-500 transition-colors hover:bg-slate-100 hover:text-slate-800 dark:text-slate-400 dark:hover:bg-slate-800 dark:hover:text-slate-200"
          >
            <RotateCcw className="h-3 w-3" />
            Reset
          </button>
        )}
      </div>

      <div className="flex items-center gap-2">
        <select
          value={query.sort ?? "created_at"}
          onChange={(e) =>
            onUpdateFilters({ sort: e.target.value as TodoSortField })
          }
          aria-label="Sort field"
          className="h-8 rounded-lg border border-slate-200 bg-slate-50 px-2.5 text-xs font-medium text-slate-700 focus:border-blue-500 focus:outline-none dark:border-slate-700 dark:bg-slate-800 dark:text-slate-200"
        >
          <option value="created_at">Sort: Created Date</option>
          <option value="due_date">Sort: Due Date</option>
          <option value="priority">Sort: Priority</option>
          <option value="updated_at">Sort: Last Updated</option>
          <option value="title">Sort: Title</option>
        </select>

        <button
          type="button"
          onClick={() =>
            onUpdateFilters({
              order: (query.order ?? "desc") === "asc" ? "desc" : ("asc" as SortOrder),
            })
          }
          title={`Order: ${(query.order ?? "desc").toUpperCase()}`}
          aria-label="Toggle sort order"
          className="inline-flex h-8 items-center gap-1.5 rounded-lg border border-slate-200 bg-slate-50 px-2.5 text-xs font-medium text-slate-700 transition-colors hover:bg-slate-100 dark:border-slate-700 dark:bg-slate-800 dark:text-slate-200 dark:hover:bg-slate-700"
        >
          <ArrowDownUp className="h-3.5 w-3.5" />
          {(query.order ?? "desc") === "asc" ? "Asc" : "Desc"}
        </button>
      </div>
    </div>
  );
}
