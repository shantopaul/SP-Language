"use client";

import React, { useEffect, useState } from "react";
import { Calendar, Clock, Trash2, X } from "lucide-react";
import { Button } from "@/components/ui/Button";
import { Input } from "@/components/ui/Input";
import { extractDatePart, formatTimestamp } from "@/lib/date";
import { ApiError } from "@/types/api";
import type { Category } from "@/types/category";
import type {
  Todo,
  TodoPriority,
  TodoStatus,
  UpdateTodoInput,
} from "@/types/todo";

interface TodoDrawerProps {
  todo: Todo | null;
  categories: Category[];
  onClose: () => void;
  onSave: (id: string, input: UpdateTodoInput) => Promise<Todo>;
  onDelete: (id: string) => Promise<boolean>;
}

export function TodoDrawer({
  todo,
  categories,
  onClose,
  onSave,
  onDelete,
}: TodoDrawerProps) {
  const [title, setTitle] = useState<string>("");
  const [description, setDescription] = useState<string>("");
  const [status, setStatus] = useState<TodoStatus>("todo");
  const [priority, setPriority] = useState<TodoPriority>("medium");
  const [categoryId, setCategoryId] = useState<string>("");
  const [dueDate, setDueDate] = useState<string>("");
  const [errorMsg, setErrorMsg] = useState<string>("");
  const [saving, setSaving] = useState<boolean>(false);
  const [deleting, setDeleting] = useState<boolean>(false);

  useEffect(() => {
    if (todo) {
      setTitle(todo.title);
      setDescription(todo.description);
      setStatus(todo.status);
      setPriority(todo.priority);
      setCategoryId(todo.category_id ?? "");
      setDueDate(extractDatePart(todo.due_date) ?? "");
      setErrorMsg("");
    }
  }, [todo]);

  if (!todo) {
    return null;
  }

  const handleSave = async (e: React.FormEvent) => {
    e.preventDefault();
    const trimmedTitle = title.trim();
    if (!trimmedTitle) {
      setErrorMsg("Title cannot be empty.");
      return;
    }
    setSaving(true);
    setErrorMsg("");
    try {
      await onSave(todo.id, {
        title: trimmedTitle,
        description,
        status,
        priority,
        category_id: categoryId || null,
        due_date: dueDate || null,
      });
      onClose();
    } catch (err) {
      if (err instanceof ApiError) {
        setErrorMsg(err.detail);
      } else {
        setErrorMsg("Could not save task changes.");
      }
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async () => {
    setDeleting(true);
    const ok = await onDelete(todo.id);
    setDeleting(false);
    if (ok) {
      onClose();
    }
  };

  return (
    <div
      className="fixed inset-0 z-40 flex justify-end"
      role="dialog"
      aria-modal="true"
      aria-label="Task details"
    >
      <div
        className="fixed inset-0 bg-slate-950/45 backdrop-blur-sm"
        onClick={onClose}
      />

      <div className="relative z-10 flex h-full w-full max-w-lg flex-col border-l border-slate-200 bg-white shadow-2xl dark:border-slate-800 dark:bg-slate-900">
        <div className="flex items-center justify-between border-b border-slate-200 px-6 py-4 dark:border-slate-800">
          <div>
            <p className="font-mono text-[11px] uppercase tracking-wider text-slate-400">
              Task Details
            </p>
            <h2 className="text-base font-bold text-slate-900 dark:text-white">
              Edit Task
            </h2>
          </div>
          <button
            type="button"
            onClick={onClose}
            aria-label="Close detail drawer"
            className="rounded-lg p-1.5 text-slate-400 hover:bg-slate-100 hover:text-slate-700 dark:hover:bg-slate-800 dark:hover:text-slate-200"
          >
            <X className="h-4 w-4" />
          </button>
        </div>

        <form
          onSubmit={handleSave}
          className="flex flex-1 flex-col justify-between overflow-y-auto p-6"
        >
          <div className="space-y-5">
            {errorMsg && (
              <div
                role="alert"
                className="rounded-xl border border-rose-200 bg-rose-50 px-3.5 py-2.5 text-xs font-medium text-rose-700 dark:border-rose-900/60 dark:bg-rose-950/50 dark:text-rose-300"
              >
                {errorMsg}
              </div>
            )}

            <Input
              label="Title"
              value={title}
              onChange={(e) => setTitle(e.target.value)}
              maxLength={200}
              required
            />

            <div className="space-y-1.5">
              <label
                htmlFor="drawer-desc"
                className="block text-xs font-semibold uppercase tracking-wide text-slate-600 dark:text-slate-400"
              >
                Notes &amp; Description
              </label>
              <textarea
                id="drawer-desc"
                rows={4}
                value={description}
                onChange={(e) => setDescription(e.target.value)}
                maxLength={4000}
                placeholder="Add implementation details, links, or checklists..."
                className="w-full rounded-lg border border-slate-200 bg-white px-3 py-2 text-sm text-slate-900 placeholder:text-slate-400 focus:border-blue-500 focus:outline-none focus:ring-2 focus:ring-blue-500/20 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-100"
              />
            </div>

            <div className="grid grid-cols-1 gap-4 sm:grid-cols-2">
              <div className="space-y-1.5">
                <label
                  htmlFor="drawer-status"
                  className="block text-xs font-semibold uppercase tracking-wide text-slate-600 dark:text-slate-400"
                >
                  Status
                </label>
                <select
                  id="drawer-status"
                  value={status}
                  onChange={(e) => setStatus(e.target.value as TodoStatus)}
                  className="h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-2 focus:ring-blue-500/20 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-100"
                >
                  <option value="todo">To Do</option>
                  <option value="in_progress">In Progress</option>
                  <option value="done">Done</option>
                  <option value="archived">Archived</option>
                </select>
              </div>

              <div className="space-y-1.5">
                <label
                  htmlFor="drawer-priority"
                  className="block text-xs font-semibold uppercase tracking-wide text-slate-600 dark:text-slate-400"
                >
                  Priority
                </label>
                <select
                  id="drawer-priority"
                  value={priority}
                  onChange={(e) => setPriority(e.target.value as TodoPriority)}
                  className="h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-2 focus:ring-blue-500/20 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-100"
                >
                  <option value="low">Low</option>
                  <option value="medium">Medium</option>
                  <option value="high">High</option>
                  <option value="urgent">Urgent</option>
                </select>
              </div>

              <div className="space-y-1.5">
                <label
                  htmlFor="drawer-category"
                  className="block text-xs font-semibold uppercase tracking-wide text-slate-600 dark:text-slate-400"
                >
                  Category
                </label>
                <select
                  id="drawer-category"
                  value={categoryId}
                  onChange={(e) => setCategoryId(e.target.value)}
                  className="h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-2 focus:ring-blue-500/20 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-100"
                >
                  <option value="">Uncategorized</option>
                  {categories.map((cat) => (
                    <option key={cat.id} value={cat.id}>
                      {cat.name}
                    </option>
                  ))}
                </select>
              </div>

              <div className="space-y-1.5">
                <div className="flex items-center justify-between">
                  <label
                    htmlFor="drawer-due"
                    className="block text-xs font-semibold uppercase tracking-wide text-slate-600 dark:text-slate-400"
                  >
                    Due Date
                  </label>
                  {dueDate && (
                    <button
                      type="button"
                      onClick={() => setDueDate("")}
                      className="text-[11px] font-medium text-blue-600 hover:underline dark:text-blue-400"
                    >
                      Clear
                    </button>
                  )}
                </div>
                <div className="relative">
                  <Calendar className="pointer-events-none absolute left-3 top-1/2 h-4 w-4 -translate-y-1/2 text-slate-400" />
                  <input
                    id="drawer-due"
                    type="date"
                    value={dueDate}
                    onChange={(e) => setDueDate(e.target.value)}
                    className="h-10 w-full rounded-lg border border-slate-200 bg-white pl-9 pr-3 text-sm text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-2 focus:ring-blue-500/20 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-100"
                  />
                </div>
              </div>
            </div>

            <div className="space-y-1.5 rounded-xl border border-slate-100 bg-slate-50 p-3.5 text-xs text-slate-500 dark:border-slate-800 dark:bg-slate-800/50 dark:text-slate-400">
              <div className="flex items-center justify-between">
                <span className="inline-flex items-center gap-1.5">
                  <Clock className="h-3.5 w-3.5" />
                  Created
                </span>
                <span className="font-mono">{formatTimestamp(todo.created_at)}</span>
              </div>
              <div className="flex items-center justify-between">
                <span>Last Updated</span>
                <span className="font-mono">{formatTimestamp(todo.updated_at)}</span>
              </div>
              {todo.completed_at && (
                <div className="flex items-center justify-between text-emerald-600 dark:text-emerald-400">
                  <span>Completed At</span>
                  <span className="font-mono">
                    {formatTimestamp(todo.completed_at)}
                  </span>
                </div>
              )}
            </div>
          </div>

          <div className="mt-6 flex items-center justify-between border-t border-slate-200 pt-4 dark:border-slate-800">
            <Button
              type="button"
              variant="danger"
              size="sm"
              loading={deleting}
              onClick={() => void handleDelete()}
            >
              <Trash2 className="h-3.5 w-3.5" />
              Delete Task
            </Button>

            <div className="flex items-center gap-2">
              <Button type="button" variant="secondary" onClick={onClose}>
                Cancel
              </Button>
              <Button type="submit" loading={saving}>
                Save Changes
              </Button>
            </div>
          </div>
        </form>
      </div>
    </div>
  );
}
