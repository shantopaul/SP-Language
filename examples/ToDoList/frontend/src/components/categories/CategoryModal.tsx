"use client";

import React, { useEffect, useState } from "react";
import { Modal } from "@/components/ui/Modal";
import { Input } from "@/components/ui/Input";
import { Button } from "@/components/ui/Button";
import { cn } from "@/lib/cn";
import { ApiError } from "@/types/api";
import {
  PRESET_CATEGORY_COLORS,
  type Category,
  type CreateCategoryInput,
  type UpdateCategoryInput,
} from "@/types/category";

interface CategoryModalProps {
  open: boolean;
  editingCategory: Category | null;
  onClose: () => void;
  onCreate: (input: CreateCategoryInput) => Promise<Category | null>;
  onUpdate: (id: string, input: UpdateCategoryInput) => Promise<Category | null>;
}

export function CategoryModal({
  open,
  editingCategory,
  onClose,
  onCreate,
  onUpdate,
}: CategoryModalProps) {
  const [name, setName] = useState<string>("");
  const [color, setColor] = useState<string>(PRESET_CATEGORY_COLORS[0]);
  const [sortOrder, setSortOrder] = useState<string>("0");
  const [errorMsg, setErrorMsg] = useState<string>("");
  const [saving, setSaving] = useState<boolean>(false);

  useEffect(() => {
    if (open) {
      setName(editingCategory?.name ?? "");
      setColor(editingCategory?.color ?? PRESET_CATEGORY_COLORS[0]);
      setSortOrder(String(editingCategory?.sort_order ?? 0));
      setErrorMsg("");
    }
  }, [editingCategory, open]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    const trimmed = name.trim();
    if (!trimmed) {
      setErrorMsg("Category name is required.");
      return;
    }
    setSaving(true);
    setErrorMsg("");
    try {
      const orderNum = Number.parseInt(sortOrder, 10);
      const safeOrder = Number.isNaN(orderNum) || orderNum < 0 ? 0 : orderNum;
      if (editingCategory) {
        await onUpdate(editingCategory.id, {
          name: trimmed,
          color,
          sort_order: safeOrder,
        });
      } else {
        await onCreate({
          name: trimmed,
          color,
          sort_order: safeOrder,
        });
      }
      onClose();
    } catch (err) {
      if (err instanceof ApiError) {
        setErrorMsg(err.detail);
      } else {
        setErrorMsg("Could not save category.");
      }
    } finally {
      setSaving(false);
    }
  };

  return (
    <Modal
      open={open}
      onClose={onClose}
      title={editingCategory ? "Edit Category" : "New Category"}
      description="Organize your tasks into color-coded workspace collections."
    >
      <form onSubmit={handleSubmit} className="space-y-4">
        <Input
          label="Category Name"
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="e.g. Engineering, Fitness, Finance"
          maxLength={40}
          error={errorMsg || undefined}
          autoFocus
        />

        <div className="space-y-2">
          <p className="text-xs font-semibold uppercase tracking-wide text-slate-600 dark:text-slate-400">
            Color Tag
          </p>
          <div className="flex flex-wrap items-center gap-2.5">
            {PRESET_CATEGORY_COLORS.map((hex) => (
              <button
                key={hex}
                type="button"
                onClick={() => setColor(hex)}
                aria-label={`Select color ${hex}`}
                className={cn(
                  "h-7 w-7 rounded-full transition-transform",
                  color === hex
                    ? "scale-110 ring-2 ring-slate-900 ring-offset-2 dark:ring-white dark:ring-offset-slate-900"
                    : "hover:scale-105"
                )}
                style={{ backgroundColor: hex }}
              />
            ))}
          </div>
        </div>

        <Input
          label="Sort Order"
          type="number"
          min={0}
          max={10000}
          value={sortOrder}
          onChange={(e) => setSortOrder(e.target.value)}
        />

        <div className="flex items-center justify-end gap-2 pt-2">
          <Button type="button" variant="secondary" onClick={onClose}>
            Cancel
          </Button>
          <Button type="submit" loading={saving}>
            {editingCategory ? "Save Category" : "Create Category"}
          </Button>
        </div>
      </form>
    </Modal>
  );
}
