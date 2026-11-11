"use client";

import { useCallback, useEffect, useState } from "react";
import { categoryApi } from "@/lib/api-client";
import { useAuth } from "@/lib/auth-context";
import { useToast } from "@/lib/toast-context";
import { ApiError } from "@/types/api";
import type {
  Category,
  CreateCategoryInput,
  UpdateCategoryInput,
} from "@/types/category";

export function useCategories() {
  const { isAuthenticated } = useAuth();
  const { pushToast } = useToast();
  const [categories, setCategories] = useState<Category[]>([]);
  const [isLoading, setIsLoading] = useState<boolean>(true);

  const refreshCategories = useCallback(async () => {
    if (!isAuthenticated) {
      setCategories([]);
      setIsLoading(false);
      return;
    }
    try {
      const data = await categoryApi.list();
      setCategories(data.items);
    } catch (err) {
      const message =
        err instanceof ApiError ? err.detail : "Failed to load categories.";
      pushToast({
        title: "Category Error",
        description: message,
        variant: "error",
      });
    } finally {
      setIsLoading(false);
    }
  }, [isAuthenticated, pushToast]);

  useEffect(() => {
    void refreshCategories();
  }, [refreshCategories]);

  const createCategory = useCallback(
    async (input: CreateCategoryInput): Promise<Category | null> => {
      try {
        const created = await categoryApi.create(input);
        setCategories((prev) =>
          [...prev, created].sort(
            (a, b) => a.sort_order - b.sort_order || a.name.localeCompare(b.name)
          )
        );
        pushToast({
          title: "Category created",
          description: `"${created.name}" is ready to use.`,
          variant: "success",
        });
        return created;
      } catch (err) {
        const message =
          err instanceof ApiError ? err.detail : "Could not create category.";
        pushToast({
          title: "Could not create category",
          description: message,
          variant: "error",
        });
        throw err;
      }
    },
    [pushToast]
  );

  const updateCategory = useCallback(
    async (id: string, input: UpdateCategoryInput): Promise<Category | null> => {
      try {
        const updated = await categoryApi.update(id, input);
        setCategories((prev) =>
          prev
            .map((cat) => (cat.id === id ? updated : cat))
            .sort(
              (a, b) => a.sort_order - b.sort_order || a.name.localeCompare(b.name)
            )
        );
        pushToast({
          title: "Category updated",
          variant: "success",
        });
        return updated;
      } catch (err) {
        const message =
          err instanceof ApiError ? err.detail : "Could not update category.";
        pushToast({
          title: "Could not update category",
          description: message,
          variant: "error",
        });
        throw err;
      }
    },
    [pushToast]
  );

  const deleteCategory = useCallback(
    async (id: string): Promise<boolean> => {
      const previous = categories;
      setCategories((prev) => prev.filter((cat) => cat.id !== id));
      try {
        await categoryApi.remove(id);
        pushToast({
          title: "Category removed",
          description: "Associated tasks were kept as uncategorized.",
          variant: "info",
        });
        return true;
      } catch (err) {
        setCategories(previous);
        const message =
          err instanceof ApiError ? err.detail : "Could not delete category.";
        pushToast({
          title: "Could not delete category",
          description: message,
          variant: "error",
        });
        return false;
      }
    },
    [categories, pushToast]
  );

  return {
    categories,
    isLoading,
    refreshCategories,
    createCategory,
    updateCategory,
    deleteCategory,
  };
}
