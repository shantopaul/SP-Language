"use client";

import React, { useEffect, useMemo, useRef, useState } from "react";
import { useRouter } from "next/navigation";
import { Loader2 } from "lucide-react";
import { CategoryModal } from "@/components/categories/CategoryModal";
import { Sidebar } from "@/components/layout/Sidebar";
import { Topbar } from "@/components/layout/Topbar";
import { BulkBar } from "@/components/todos/BulkBar";
import { FilterToolbar } from "@/components/todos/FilterToolbar";
import { QuickAddBar } from "@/components/todos/QuickAddBar";
import { StatsCards } from "@/components/todos/StatsCards";
import { TodoDrawer } from "@/components/todos/TodoDrawer";
import { TodoList } from "@/components/todos/TodoList";
import { useKeyboardShortcuts } from "@/hooks/useKeyboardShortcuts";
import { useCategories } from "@/hooks/useCategories";
import { useTodos } from "@/hooks/useTodos";
import { useAuth } from "@/lib/auth-context";
import type { Category } from "@/types/category";
import type { Todo } from "@/types/todo";

export default function WorkspacePage() {
  const router = useRouter();
  const { isAuthenticated, isLoading: authLoading } = useAuth();

  const {
    categories,
    refreshCategories,
    createCategory,
    updateCategory,
    deleteCategory,
  } = useCategories();

  const {
    query,
    searchInput,
    todos,
    meta,
    stats,
    isLoading: todosLoading,
    selectedIds,
    updateSearch,
    updateFilters,
    resetFilters,
    refresh,
    createTodo,
    updateTodo,
    toggleComplete,
    deleteTodo,
    runBulk,
    toggleSelect,
    toggleSelectAllVisible,
    clearSelection,
  } = useTodos(refreshCategories);

  const [mobileMenuOpen, setMobileMenuOpen] = useState<boolean>(false);
  const [categoryModalOpen, setCategoryModalOpen] = useState<boolean>(false);
  const [editingCategory, setEditingCategory] = useState<Category | null>(null);
  const [activeTodo, setActiveTodo] = useState<Todo | null>(null);

  const quickAddRef = useRef<HTMLInputElement>(null);
  const searchRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (!authLoading && !isAuthenticated) {
      router.replace("/login");
    }
  }, [authLoading, isAuthenticated, router]);

  useKeyboardShortcuts({
    onNewTask: () => quickAddRef.current?.focus(),
    onFocusSearch: () => searchRef.current?.focus(),
    onEscape: () => {
      if (activeTodo) {
        setActiveTodo(null);
      } else if (categoryModalOpen) {
        setCategoryModalOpen(false);
      } else if (mobileMenuOpen) {
        setMobileMenuOpen(false);
      }
    },
  });

  const viewTitle = useMemo(() => {
    if (query.due === "today") return "Due Today";
    if (query.due === "overdue") return "Overdue Tasks";
    if (query.category_id === "uncategorized") return "Uncategorized";
    if (query.category_id && query.category_id !== "all") {
      const cat = categories.find((c) => c.id === query.category_id);
      return cat ? cat.name : "Category Tasks";
    }
    if (query.status === "in_progress") return "In Progress";
    if (query.status === "done") return "Completed";
    if (query.status === "archived") return "Archived";
    return "All Tasks";
  }, [categories, query]);

  if (authLoading || !isAuthenticated) {
    return (
      <div className="flex min-h-screen items-center justify-center bg-slate-50 dark:bg-slate-950">
        <div className="flex items-center gap-2.5 text-sm font-medium text-slate-500">
          <Loader2 className="h-4 w-4 animate-spin text-blue-600" />
          Loading workspace...
        </div>
      </div>
    );
  }

  const allVisibleSelected =
    todos.length > 0 && todos.every((item) => selectedIds.includes(item.id));

  const hasActiveFilters =
    (query.status ?? "all") !== "all" ||
    (query.priority ?? "all") !== "all" ||
    (query.category_id ?? "all") !== "all" ||
    (query.due ?? "all") !== "all" ||
    (query.q ?? "").trim().length > 0;

  return (
    <div className="flex min-h-screen bg-slate-50 dark:bg-slate-950">
      <Sidebar
        stats={stats}
        categories={categories}
        query={query}
        onSelectView={updateFilters}
        onNewCategory={() => {
          setEditingCategory(null);
          setCategoryModalOpen(true);
        }}
        onEditCategory={(cat) => {
          setEditingCategory(cat);
          setCategoryModalOpen(true);
        }}
        onDeleteCategory={(cat) => {
          void deleteCategory(cat.id).then((ok) => {
            if (ok) {
              if (query.category_id === cat.id) {
                updateFilters({ category_id: "all" });
              } else {
                void refresh();
              }
            }
          });
        }}
        mobileOpen={mobileMenuOpen}
        onCloseMobile={() => setMobileMenuOpen(false)}
      />

      <div className="flex min-w-0 flex-1 flex-col">
        <Topbar
          title={viewTitle}
          searchValue={searchInput}
          onSearchChange={updateSearch}
          searchInputRef={searchRef}
          onOpenMobileMenu={() => setMobileMenuOpen(true)}
        />

        <main className="mx-auto w-full max-w-5xl flex-1 space-y-5 px-4 py-6 sm:px-6">
          <StatsCards
            stats={stats}
            query={query}
            onQuickFilter={updateFilters}
          />

          <QuickAddBar
            categories={categories}
            defaultCategoryId={query.category_id}
            inputRef={quickAddRef}
            onCreate={createTodo}
          />

          <FilterToolbar
            query={query}
            searchValue={searchInput}
            totalVisible={todos.length}
            allVisibleSelected={allVisibleSelected}
            onToggleSelectAll={toggleSelectAllVisible}
            onSearchChange={updateSearch}
            onUpdateFilters={updateFilters}
            onResetFilters={resetFilters}
          />

          <BulkBar
            selectedCount={selectedIds.length}
            onRunBulk={runBulk}
            onClearSelection={clearSelection}
          />

          <TodoList
            todos={todos}
            meta={meta}
            isLoading={todosLoading}
            hasActiveFilters={hasActiveFilters}
            selectedIds={selectedIds}
            onToggleSelect={toggleSelect}
            onToggleComplete={toggleComplete}
            onOpenDetail={setActiveTodo}
            onDelete={deleteTodo}
            onPageChange={(page) => updateFilters({ page })}
            onResetFilters={resetFilters}
            onFocusQuickAdd={() => quickAddRef.current?.focus()}
          />
        </main>
      </div>

      <CategoryModal
        open={categoryModalOpen}
        editingCategory={editingCategory}
        onClose={() => setCategoryModalOpen(false)}
        onCreate={createCategory}
        onUpdate={async (id, input) => {
          const updated = await updateCategory(id, input);
          if (updated) {
            await refresh();
          }
          return updated;
        }}
      />

      <TodoDrawer
        todo={activeTodo}
        categories={categories}
        onClose={() => setActiveTodo(null)}
        onSave={updateTodo}
        onDelete={deleteTodo}
      />
    </div>
  );
}
