"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import { todoApi } from "@/lib/api-client";
import { useAuth } from "@/lib/auth-context";
import { useToast } from "@/lib/toast-context";
import { ApiError } from "@/types/api";
import type {
  BulkAction,
  CreateTodoInput,
  PaginationMeta,
  Todo,
  TodoPriority,
  TodoQuery,
  TodoStats,
  TodoStatus,
  UpdateTodoInput,
} from "@/types/todo";

const EMPTY_STATS: TodoStats = {
  total: 0,
  todo: 0,
  in_progress: 0,
  done: 0,
  archived: 0,
  overdue: 0,
  due_today: 0,
  urgent: 0,
  high: 0,
};

const EMPTY_META: PaginationMeta = {
  total: 0,
  page: 1,
  limit: 20,
  total_pages: 1,
  has_next: false,
  has_prev: false,
};

export function useTodos(onCategoriesDirty?: () => Promise<void>) {
  const { isAuthenticated } = useAuth();
  const { pushToast } = useToast();

  const [query, setQuery] = useState<TodoQuery>({
    status: "all",
    priority: "all",
    category_id: "all",
    q: "",
    due: "all",
    sort: "created_at",
    order: "desc",
    page: 1,
    limit: 20,
  });

  const [searchInput, setSearchInput] = useState<string>("");
  const [todos, setTodos] = useState<Todo[]>([]);
  const [meta, setMeta] = useState<PaginationMeta>(EMPTY_META);
  const [stats, setStats] = useState<TodoStats>(EMPTY_STATS);
  const [isLoading, setIsLoading] = useState<boolean>(true);
  const [selectedIds, setSelectedIds] = useState<string[]>([]);
  const searchTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const fetchAll = useCallback(
    async (activeQuery: TodoQuery) => {
      if (!isAuthenticated) {
        setTodos([]);
        setMeta(EMPTY_META);
        setStats(EMPTY_STATS);
        setIsLoading(false);
        return;
      }
      try {
        const [pageRes, statsRes] = await Promise.all([
          todoApi.list(activeQuery),
          todoApi.stats(),
        ]);
        setTodos(pageRes.items);
        setMeta(pageRes.meta);
        setStats(statsRes);
      } catch (err) {
        const message =
          err instanceof ApiError ? err.detail : "Failed to load tasks.";
        pushToast({
          title: "Load failed",
          description: message,
          variant: "error",
        });
      } finally {
        setIsLoading(false);
      }
    },
    [isAuthenticated, pushToast]
  );

  useEffect(() => {
    void fetchAll(query);
  }, [fetchAll, query]);

  const updateSearch = useCallback((value: string) => {
    setSearchInput(value);
    if (searchTimerRef.current) {
      clearTimeout(searchTimerRef.current);
    }
    searchTimerRef.current = setTimeout(() => {
      setQuery((prev) => ({ ...prev, q: value, page: 1 }));
    }, 240);
  }, []);

  useEffect(() => {
    return () => {
      if (searchTimerRef.current) {
        clearTimeout(searchTimerRef.current);
      }
    };
  }, []);

  const updateFilters = useCallback((patch: Partial<TodoQuery>) => {
    setQuery((prev) => ({
      ...prev,
      ...patch,
      page: patch.page !== undefined ? patch.page : 1,
    }));
  }, []);

  const resetFilters = useCallback(() => {
    setSearchInput("");
    setQuery({
      status: "all",
      priority: "all",
      category_id: "all",
      q: "",
      due: "all",
      sort: "created_at",
      order: "desc",
      page: 1,
      limit: 20,
    });
  }, []);

  const refresh = useCallback(async () => {
    await fetchAll(query);
    if (onCategoriesDirty) {
      await onCategoriesDirty();
    }
  }, [fetchAll, onCategoriesDirty, query]);

  const createTodo = useCallback(
    async (input: CreateTodoInput): Promise<Todo> => {
      const created = await todoApi.create(input);
      pushToast({
        title: "Task added",
        description: created.title,
        variant: "success",
      });
      await fetchAll(query);
      if (onCategoriesDirty) {
        await onCategoriesDirty();
      }
      return created;
    },
    [fetchAll, onCategoriesDirty, pushToast, query]
  );

  const updateTodo = useCallback(
    async (id: string, input: UpdateTodoInput): Promise<Todo> => {
      const previous = todos;
      setTodos((prev) =>
        prev.map((item) =>
          item.id === id
            ? {
                ...item,
                ...input,
                category_id:
                  input.category_id !== undefined
                    ? input.category_id
                    : item.category_id,
                due_date:
                  input.due_date !== undefined ? input.due_date : item.due_date,
              }
            : item
        )
      );
      try {
        const updated = await todoApi.update(id, input);
        setTodos((prev) =>
          prev.map((item) => (item.id === id ? updated : item))
        );
        await fetchAll(query);
        if (onCategoriesDirty) {
          await onCategoriesDirty();
        }
        return updated;
      } catch (err) {
        setTodos(previous);
        const message =
          err instanceof ApiError ? err.detail : "Could not update task.";
        pushToast({
          title: "Update failed",
          description: message,
          variant: "error",
        });
        throw err;
      }
    },
    [fetchAll, onCategoriesDirty, pushToast, query, todos]
  );

  const toggleComplete = useCallback(
    async (todo: Todo): Promise<void> => {
      const nextStatus: TodoStatus = todo.status === "done" ? "todo" : "done";
      await updateTodo(todo.id, { status: nextStatus });
    },
    [updateTodo]
  );

  const deleteTodo = useCallback(
    async (id: string): Promise<boolean> => {
      const previous = todos;
      setTodos((prev) => prev.filter((item) => item.id !== id));
      setSelectedIds((prev) => prev.filter((itemId) => itemId !== id));
      try {
        await todoApi.remove(id);
        pushToast({
          title: "Task deleted",
          variant: "info",
        });
        await fetchAll(query);
        if (onCategoriesDirty) {
          await onCategoriesDirty();
        }
        return true;
      } catch (err) {
        setTodos(previous);
        const message =
          err instanceof ApiError ? err.detail : "Could not delete task.";
        pushToast({
          title: "Delete failed",
          description: message,
          variant: "error",
        });
        return false;
      }
    },
    [fetchAll, onCategoriesDirty, pushToast, query, todos]
  );

  const runBulk = useCallback(
    async (action: BulkAction, priority?: TodoPriority): Promise<void> => {
      if (selectedIds.length === 0) {
        return;
      }
      try {
        const res = await todoApi.bulk({
          action,
          ids: selectedIds,
          priority,
        });
        setSelectedIds([]);
        pushToast({
          title: "Bulk update complete",
          description: `Updated ${res.affected} task${res.affected === 1 ? "" : "s"}.`,
          variant: "success",
        });
        await fetchAll(query);
        if (onCategoriesDirty) {
          await onCategoriesDirty();
        }
      } catch (err) {
        const message =
          err instanceof ApiError ? err.detail : "Bulk action failed.";
        pushToast({
          title: "Bulk action failed",
          description: message,
          variant: "error",
        });
      }
    },
    [fetchAll, onCategoriesDirty, pushToast, query, selectedIds]
  );

  const toggleSelect = useCallback((id: string) => {
    setSelectedIds((prev) =>
      prev.includes(id) ? prev.filter((x) => x !== id) : [...prev, id]
    );
  }, []);

  const toggleSelectAllVisible = useCallback(() => {
    const visibleIds = todos.map((t) => t.id);
    const allSelected =
      visibleIds.length > 0 && visibleIds.every((id) => selectedIds.includes(id));
    setSelectedIds(allSelected ? [] : visibleIds);
  }, [selectedIds, todos]);

  const clearSelection = useCallback(() => {
    setSelectedIds([]);
  }, []);

  return {
    query,
    searchInput,
    todos,
    meta,
    stats,
    isLoading,
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
  };
}
