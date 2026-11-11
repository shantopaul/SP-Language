"use client";

import React from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import {
  AlertTriangle,
  Archive,
  CalendarClock,
  CheckCircle2,
  FolderPlus,
  Inbox,
  Layers,
  Pencil,
  Settings,
  Trash2,
  X,
  Zap,
} from "lucide-react";
import { cn } from "@/lib/cn";
import type { Category } from "@/types/category";
import type { TodoQuery, TodoStats } from "@/types/todo";

interface SidebarProps {
  stats: TodoStats;
  categories: Category[];
  query: TodoQuery;
  onSelectView: (patch: Partial<TodoQuery>) => void;
  onNewCategory: () => void;
  onEditCategory: (category: Category) => void;
  onDeleteCategory: (category: Category) => void;
  mobileOpen: boolean;
  onCloseMobile: () => void;
}

export function Sidebar({
  stats,
  categories,
  query,
  onSelectView,
  onNewCategory,
  onEditCategory,
  onDeleteCategory,
  mobileOpen,
  onCloseMobile,
}: SidebarProps) {
  const pathname = usePathname();
  const isWorkspace = pathname === "/";

  const isAllActive =
    isWorkspace &&
    (query.status ?? "all") === "all" &&
    (query.due ?? "all") === "all" &&
    (query.category_id ?? "all") === "all";

  const isTodayActive = isWorkspace && query.due === "today";
  const isOverdueActive = isWorkspace && query.due === "overdue";
  const isInProgressActive =
    isWorkspace && query.status === "in_progress" && (query.due ?? "all") === "all";
  const isDoneActive =
    isWorkspace && query.status === "done" && (query.due ?? "all") === "all";
  const isArchivedActive =
    isWorkspace && query.status === "archived" && (query.due ?? "all") === "all";

  const applyView = (patch: Partial<TodoQuery>) => {
    onSelectView({
      status: "all",
      due: "all",
      category_id: "all",
      ...patch,
      page: 1,
    });
    onCloseMobile();
  };

  const navContent = (
    <div className="flex h-full flex-col bg-white dark:bg-slate-900">
      <div className="flex h-16 items-center justify-between border-b border-slate-200 px-5 dark:border-slate-800">
        <Link href="/" className="flex items-center gap-2.5" onClick={onCloseMobile}>
          <div className="flex h-8 w-8 items-center justify-center rounded-lg bg-blue-600 text-white shadow-sm">
            <CheckCircle2 className="h-5 w-5" />
          </div>
          <div>
            <span className="block text-sm font-bold tracking-tight text-slate-900 dark:text-white">
              TaskFlow
            </span>
            <span className="block font-mono text-[10px] font-medium text-slate-400">
              SP 4.0 · SQLite
            </span>
          </div>
        </Link>
        <button
          type="button"
          onClick={onCloseMobile}
          aria-label="Close navigation menu"
          className="rounded-lg p-1.5 text-slate-400 hover:bg-slate-100 hover:text-slate-700 lg:hidden dark:hover:bg-slate-800"
        >
          <X className="h-4 w-4" />
        </button>
      </div>

      <div className="flex-1 space-y-6 overflow-y-auto px-3 py-4">
        <div>
          <p className="mb-2 px-2.5 text-[11px] font-semibold uppercase tracking-wider text-slate-400">
            Views
          </p>
          <nav className="space-y-0.5" aria-label="Task views">
            <SidebarItem
              active={isAllActive}
              icon={<Inbox className="h-4 w-4" />}
              label="All Tasks"
              count={stats.total}
              onClick={() => applyView({})}
            />
            <SidebarItem
              active={isTodayActive}
              icon={<CalendarClock className="h-4 w-4 text-blue-500" />}
              label="Due Today"
              count={stats.due_today}
              onClick={() => applyView({ due: "today" })}
            />
            <SidebarItem
              active={isOverdueActive}
              icon={<AlertTriangle className="h-4 w-4 text-rose-500" />}
              label="Overdue"
              count={stats.overdue}
              highlightCount={stats.overdue > 0}
              onClick={() => applyView({ due: "overdue" })}
            />
            <SidebarItem
              active={isInProgressActive}
              icon={<Zap className="h-4 w-4 text-amber-500" />}
              label="In Progress"
              count={stats.in_progress}
              onClick={() => applyView({ status: "in_progress" })}
            />
            <SidebarItem
              active={isDoneActive}
              icon={<CheckCircle2 className="h-4 w-4 text-emerald-500" />}
              label="Completed"
              count={stats.done}
              onClick={() => applyView({ status: "done" })}
            />
            <SidebarItem
              active={isArchivedActive}
              icon={<Archive className="h-4 w-4 text-slate-400" />}
              label="Archived"
              count={stats.archived}
              onClick={() => applyView({ status: "archived" })}
            />
          </nav>
        </div>

        <div>
          <div className="mb-2 flex items-center justify-between px-2.5">
            <p className="text-[11px] font-semibold uppercase tracking-wider text-slate-400">
              Categories
            </p>
            <button
              type="button"
              onClick={onNewCategory}
              title="Add category"
              aria-label="Add new category"
              className="rounded-md p-1 text-slate-400 transition-colors hover:bg-slate-100 hover:text-blue-600 dark:hover:bg-slate-800 dark:hover:text-blue-400"
            >
              <FolderPlus className="h-3.5 w-3.5" />
            </button>
          </div>
          <div className="space-y-0.5">
            {categories.map((cat) => {
              const active = isWorkspace && query.category_id === cat.id;
              return (
                <div
                  key={cat.id}
                  className={cn(
                    "group flex items-center justify-between rounded-lg px-2.5 py-2 text-sm transition-colors",
                    active
                      ? "bg-blue-50 font-medium text-blue-700 dark:bg-blue-950/60 dark:text-blue-300"
                      : "text-slate-700 hover:bg-slate-100 dark:text-slate-300 dark:hover:bg-slate-800/70"
                  )}
                >
                  <button
                    type="button"
                    onClick={() => applyView({ category_id: cat.id })}
                    className="flex min-w-0 flex-1 items-center gap-2.5 text-left"
                  >
                    <span
                      className="h-2.5 w-2.5 shrink-0 rounded-full"
                      style={{ backgroundColor: cat.color }}
                    />
                    <span className="truncate">{cat.name}</span>
                  </button>
                  <div className="flex items-center gap-1">
                    <button
                      type="button"
                      onClick={(e) => {
                        e.stopPropagation();
                        onEditCategory(cat);
                      }}
                      aria-label={`Edit category ${cat.name}`}
                      className="hidden rounded p-1 text-slate-400 hover:bg-white hover:text-slate-700 group-hover:inline-flex dark:hover:bg-slate-700 dark:hover:text-slate-200"
                    >
                      <Pencil className="h-3 w-3" />
                    </button>
                    <button
                      type="button"
                      onClick={(e) => {
                        e.stopPropagation();
                        onDeleteCategory(cat);
                      }}
                      aria-label={`Delete category ${cat.name}`}
                      className="hidden rounded p-1 text-slate-400 hover:bg-white hover:text-rose-600 group-hover:inline-flex dark:hover:bg-slate-700 dark:hover:text-rose-400"
                    >
                      <Trash2 className="h-3 w-3" />
                    </button>
                    <span className="ml-1 font-mono text-xs text-slate-400 group-hover:hidden">
                      {cat.todo_count}
                    </span>
                  </div>
                </div>
              );
            })}

            <button
              type="button"
              onClick={() => applyView({ category_id: "uncategorized" })}
              className={cn(
                "flex w-full items-center gap-2.5 rounded-lg px-2.5 py-2 text-left text-sm transition-colors",
                isWorkspace && query.category_id === "uncategorized"
                  ? "bg-blue-50 font-medium text-blue-700 dark:bg-blue-950/60 dark:text-blue-300"
                  : "text-slate-500 hover:bg-slate-100 hover:text-slate-800 dark:text-slate-400 dark:hover:bg-slate-800/70"
              )}
            >
              <Layers className="h-3.5 w-3.5 shrink-0" />
              <span className="truncate">Uncategorized</span>
            </button>
          </div>
        </div>
      </div>

      <div className="border-t border-slate-200 p-3 dark:border-slate-800">
        <Link
          href="/settings"
          onClick={onCloseMobile}
          className={cn(
            "flex items-center gap-2.5 rounded-lg px-3 py-2 text-sm font-medium transition-colors",
            pathname === "/settings"
              ? "bg-blue-50 text-blue-700 dark:bg-blue-950/60 dark:text-blue-300"
              : "text-slate-600 hover:bg-slate-100 hover:text-slate-900 dark:text-slate-400 dark:hover:bg-slate-800 dark:hover:text-slate-100"
          )}
        >
          <Settings className="h-4 w-4" />
          Settings &amp; Profile
        </Link>
      </div>
    </div>
  );

  return (
    <>
      <aside className="hidden w-64 shrink-0 border-r border-slate-200 lg:block dark:border-slate-800">
        {navContent}
      </aside>

      {mobileOpen && (
        <div
          className="fixed inset-0 z-40 flex lg:hidden"
          role="dialog"
          aria-modal="true"
          aria-label="Mobile navigation"
        >
          <div
            className="fixed inset-0 bg-slate-950/50 backdrop-blur-sm"
            onClick={onCloseMobile}
          />
          <aside className="relative z-10 w-72 max-w-[85vw] border-r border-slate-200 dark:border-slate-800">
            {navContent}
          </aside>
        </div>
      )}
    </>
  );
}

function SidebarItem({
  active,
  icon,
  label,
  count,
  highlightCount = false,
  onClick,
}: {
  active: boolean;
  icon: React.ReactNode;
  label: string;
  count: number;
  highlightCount?: boolean;
  onClick: () => void;
}) {
  return (
    <button
      type="button"
      onClick={onClick}
      className={cn(
        "flex w-full items-center justify-between rounded-lg px-2.5 py-2 text-sm transition-colors",
        active
          ? "bg-blue-50 font-semibold text-blue-700 dark:bg-blue-950/60 dark:text-blue-300"
          : "text-slate-700 hover:bg-slate-100 dark:text-slate-300 dark:hover:bg-slate-800/70"
      )}
    >
      <span className="flex items-center gap-2.5 truncate">
        {icon}
        <span className="truncate">{label}</span>
      </span>
      <span
        className={cn(
          "rounded-full px-2 py-0.5 font-mono text-xs",
          highlightCount
            ? "bg-rose-100 font-semibold text-rose-700 dark:bg-rose-950 dark:text-rose-300"
            : active
              ? "bg-blue-100 text-blue-700 dark:bg-blue-900/70 dark:text-blue-200"
              : "text-slate-400"
        )}
      >
        {count}
      </span>
    </button>
  );
}
