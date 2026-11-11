"use client";

import React, { useState } from "react";
import { Calendar, Folder, Plus } from "lucide-react";
import { Button } from "@/components/ui/Button";
import { ApiError } from "@/types/api";
import type { Category } from "@/types/category";
import type { CreateTodoInput, TodoPriority } from "@/types/todo";

interface QuickAddBarProps {
  categories: Category[];
  defaultCategoryId?: string;
  inputRef?: React.RefObject<HTMLInputElement>;
  onCreate: (input: CreateTodoInput) => Promise<unknown>;
}

export function QuickAddBar({
  categories,
  defaultCategoryId,
  inputRef,
  onCreate,
}: QuickAddBarProps) {
  const [title, setTitle] = useState<string>("");
  const [priority, setPriority] = useState<TodoPriority>("medium");
  const [categoryId, setCategoryId] = useState<string>("");
  const [dueDate, setDueDate] = useState<string>("");
  const [errorMsg, setErrorMsg] = useState<string>("");
  const [submitting, setSubmitting] = useState<boolean>(false);

  const effectiveCategory =
    categoryId ||
    (defaultCategoryId &&
    defaultCategoryId !== "all" &&
    defaultCategoryId !== "uncategorized"
      ? defaultCategoryId
      : "");

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    const trimmed = title.trim();
    if (!trimmed) {
      setErrorMsg("Enter a task title first.");
      return;
    }
    setSubmitting(true);
    setErrorMsg("");
    try {
      await onCreate({
        title: trimmed,
        priority,
        category_id: effectiveCategory || null,
        due_date: dueDate || null,
      });
      setTitle("");
      setDueDate("");
    } catch (err) {
      if (err instanceof ApiError) {
        setErrorMsg(err.detail);
      } else {
        setErrorMsg("Could not add task.");
      }
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <form
      onSubmit={handleSubmit}
      className="rounded-xl border border-slate-200 bg-white p-3 shadow-soft dark:border-slate-800 dark:bg-slate-900"
    >
      <div className="flex flex-col gap-2.5 sm:flex-row sm:items-center">
        <div className="flex min-w-0 flex-1 items-center gap-2.5 px-1">
          <Plus className="h-4 w-4 shrink-0 text-blue-600 dark:text-blue-400" />
          <input
            ref={inputRef}
            type="text"
            value={title}
            onChange={(e) => {
              setTitle(e.target.value);
              if (errorMsg) setErrorMsg("");
            }}
            placeholder="Add a new task... (press 'n' anywhere to focus)"
            aria-label="New task title"
            maxLength={200}
            className="w-full bg-transparent text-sm text-slate-900 placeholder:text-slate-400 focus:outline-none dark:text-slate-100"
          />
        </div>

        <div className="flex flex-wrap items-center gap-2 border-t border-slate-100 pt-2 sm:border-t-0 sm:pt-0 dark:border-slate-800">
          <select
            value={priority}
            onChange={(e) => setPriority(e.target.value as TodoPriority)}
            aria-label="Task priority"
            className="h-9 rounded-lg border border-slate-200 bg-slate-50 px-2.5 text-xs font-medium text-slate-700 focus:border-blue-500 focus:outline-none dark:border-slate-700 dark:bg-slate-800 dark:text-slate-200"
          >
            <option value="low">Low priority</option>
            <option value="medium">Medium</option>
            <option value="high">High</option>
            <option value="urgent">Urgent</option>
          </select>

          <div className="relative flex items-center">
            <Folder className="pointer-events-none absolute left-2.5 h-3.5 w-3.5 text-slate-400" />
            <select
              value={effectiveCategory}
              onChange={(e) => setCategoryId(e.target.value)}
              aria-label="Task category"
              className="h-9 rounded-lg border border-slate-200 bg-slate-50 pl-7 pr-2.5 text-xs font-medium text-slate-700 focus:border-blue-500 focus:outline-none dark:border-slate-700 dark:bg-slate-800 dark:text-slate-200"
            >
              <option value="">No category</option>
              {categories.map((cat) => (
                <option key={cat.id} value={cat.id}>
                  {cat.name}
                </option>
              ))}
            </select>
          </div>

          <div className="relative flex items-center">
            <Calendar className="pointer-events-none absolute left-2.5 h-3.5 w-3.5 text-slate-400" />
            <input
              type="date"
              value={dueDate}
              onChange={(e) => setDueDate(e.target.value)}
              aria-label="Due date"
              className="h-9 rounded-lg border border-slate-200 bg-slate-50 pl-7 pr-2 text-xs text-slate-700 focus:border-blue-500 focus:outline-none dark:border-slate-700 dark:bg-slate-800 dark:text-slate-200"
            />
          </div>

          <Button type="submit" size="sm" loading={submitting} className="h-9 px-4">
            Add Task
          </Button>
        </div>
      </div>

      {errorMsg && (
        <p className="mt-2 px-1 text-xs font-medium text-rose-600 dark:text-rose-400">
          {errorMsg}
        </p>
      )}
    </form>
  );
}
