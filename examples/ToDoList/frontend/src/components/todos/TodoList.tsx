"use client";

import React from "react";
import { CheckCircle2, ChevronLeft, ChevronRight, Sparkles } from "lucide-react";
import { Button } from "@/components/ui/Button";
import { TodoListSkeleton } from "@/components/ui/Skeleton";
import { TodoRow } from "@/components/todos/TodoRow";
import type { PaginationMeta, Todo } from "@/types/todo";

interface TodoListProps {
  todos: Todo[];
  meta: PaginationMeta;
  isLoading: boolean;
  hasActiveFilters: boolean;
  selectedIds: string[];
  onToggleSelect: (id: string) => void;
  onToggleComplete: (todo: Todo) => Promise<void>;
  onOpenDetail: (todo: Todo) => void;
  onDelete: (id: string) => Promise<boolean>;
  onPageChange: (page: number) => void;
  onResetFilters: () => void;
  onFocusQuickAdd: () => void;
}

export function TodoList({
  todos,
  meta,
  isLoading,
  hasActiveFilters,
  selectedIds,
  onToggleSelect,
  onToggleComplete,
  onOpenDetail,
  onDelete,
  onPageChange,
  onResetFilters,
  onFocusQuickAdd,
}: TodoListProps) {
  if (isLoading) {
    return <TodoListSkeleton />;
  }

  if (todos.length === 0) {
    return (
      <div className="flex flex-col items-center justify-center rounded-2xl border border-dashed border-slate-300 bg-white px-6 py-14 text-center dark:border-slate-700 dark:bg-slate-900/70">
        <div className="mb-3 flex h-12 w-12 items-center justify-center rounded-2xl bg-slate-100 text-slate-500 dark:bg-slate-800 dark:text-slate-400">
          {hasActiveFilters ? (
            <Sparkles className="h-5 w-5" />
          ) : (
            <CheckCircle2 className="h-5 w-5 text-emerald-500" />
          )}
        </div>
        <h3 className="text-sm font-semibold text-slate-900 dark:text-slate-100">
          {hasActiveFilters ? "No tasks match these filters" : "Your task list is clear"}
        </h3>
        <p className="mt-1 max-w-sm text-xs text-slate-500 dark:text-slate-400">
          {hasActiveFilters
            ? "Try broadening your search query, status filter, or selected category."
            : "Add your first task above or press 'n' anywhere to jump straight into Quick Add."}
        </p>
        <div className="mt-4">
          {hasActiveFilters ? (
            <Button size="sm" variant="secondary" onClick={onResetFilters}>
              Clear all filters
            </Button>
          ) : (
            <Button size="sm" onClick={onFocusQuickAdd}>
              Add First Task
            </Button>
          )}
        </div>
      </div>
    );
  }

  return (
    <div className="space-y-3">
      <div className="space-y-2">
        {todos.map((todo) => (
          <TodoRow
            key={todo.id}
            todo={todo}
            selected={selectedIds.includes(todo.id)}
            onToggleSelect={onToggleSelect}
            onToggleComplete={onToggleComplete}
            onOpenDetail={onOpenDetail}
            onDelete={onDelete}
          />
        ))}
      </div>

      {meta.total > 0 && (
        <div className="flex flex-wrap items-center justify-between gap-3 rounded-xl border border-slate-200/80 bg-white px-4 py-2.5 text-xs text-slate-600 dark:border-slate-800 dark:bg-slate-900 dark:text-slate-400">
          <span>
            Showing{" "}
            <strong className="font-semibold text-slate-900 dark:text-slate-200">
              {todos.length}
            </strong>{" "}
            of{" "}
            <strong className="font-semibold text-slate-900 dark:text-slate-200">
              {meta.total}
            </strong>{" "}
            task{meta.total === 1 ? "" : "s"} · Page{" "}
            <span className="font-mono">
              {meta.page}/{meta.total_pages}
            </span>
          </span>

          <div className="flex items-center gap-1.5">
            <Button
              size="sm"
              variant="secondary"
              disabled={!meta.has_prev}
              onClick={() => onPageChange(meta.page - 1)}
            >
              <ChevronLeft className="h-3.5 w-3.5" />
              Prev
            </Button>
            <Button
              size="sm"
              variant="secondary"
              disabled={!meta.has_next}
              onClick={() => onPageChange(meta.page + 1)}
            >
              Next
              <ChevronRight className="h-3.5 w-3.5" />
            </Button>
          </div>
        </div>
      )}
    </div>
  );
}
