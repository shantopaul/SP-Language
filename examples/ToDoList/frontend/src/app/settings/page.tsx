"use client";

import React, { useEffect, useState } from "react";
import Link from "next/link";
import { useRouter } from "next/navigation";
import {
  Activity,
  ArrowLeft,
  FolderPlus,
  Loader2,
  Moon,
  Pencil,
  Server,
  Sun,
  Trash2,
  UserCheck,
} from "lucide-react";
import { CategoryModal } from "@/components/categories/CategoryModal";
import { Topbar } from "@/components/layout/Topbar";
import { Button } from "@/components/ui/Button";
import { Input } from "@/components/ui/Input";
import { useCategories } from "@/hooks/useCategories";
import { useTheme } from "@/hooks/useTheme";
import { healthApi } from "@/lib/api-client";
import { useAuth } from "@/lib/auth-context";
import { API_BASE_URL } from "@/lib/config";
import { formatTimestamp } from "@/lib/date";
import { useToast } from "@/lib/toast-context";
import { ApiError, type HealthResponse } from "@/types/api";
import type { Category } from "@/types/category";

export default function SettingsPage() {
  const router = useRouter();
  const { user, isAuthenticated, isLoading: authLoading, updateName } = useAuth();
  const { theme, toggleTheme } = useTheme();
  const { pushToast } = useToast();
  const { categories, createCategory, updateCategory, deleteCategory } =
    useCategories();

  const [name, setName] = useState<string>("");
  const [nameError, setNameError] = useState<string>("");
  const [savingProfile, setSavingProfile] = useState<boolean>(false);
  const [health, setHealth] = useState<HealthResponse | null>(null);
  const [healthError, setHealthError] = useState<string>("");
  const [categoryModalOpen, setCategoryModalOpen] = useState<boolean>(false);
  const [editingCategory, setEditingCategory] = useState<Category | null>(null);

  useEffect(() => {
    if (!authLoading && !isAuthenticated) {
      router.replace("/login");
    }
  }, [authLoading, isAuthenticated, router]);

  useEffect(() => {
    if (user) {
      setName(user.name);
    }
  }, [user]);

  useEffect(() => {
    void healthApi
      .check()
      .then((res) => {
        setHealth(res);
        setHealthError("");
      })
      .catch(() => {
        setHealthError("Backend health probe did not respond.");
      });
  }, []);

  const handleSaveProfile = async (e: React.FormEvent) => {
    e.preventDefault();
    const trimmed = name.trim();
    if (trimmed.length < 2) {
      setNameError("Name must be at least 2 characters.");
      return;
    }
    setSavingProfile(true);
    setNameError("");
    try {
      await updateName(trimmed);
      pushToast({ title: "Profile saved", variant: "success" });
    } catch (err) {
      if (err instanceof ApiError) {
        setNameError(err.detail);
      } else {
        setNameError("Failed to update profile.");
      }
    } finally {
      setSavingProfile(false);
    }
  };

  if (authLoading || !isAuthenticated || !user) {
    return (
      <div className="flex min-h-screen items-center justify-center bg-slate-50 dark:bg-slate-950">
        <Loader2 className="h-5 w-5 animate-spin text-blue-600" />
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-slate-50 dark:bg-slate-950">
      <Topbar title="Settings & Profile" showSearch={false} />

      <main className="mx-auto max-w-3xl space-y-6 px-4 py-8 sm:px-6">
        <div>
          <Link
            href="/"
            className="inline-flex items-center gap-1.5 text-xs font-semibold text-slate-500 hover:text-blue-600 dark:text-slate-400"
          >
            <ArrowLeft className="h-3.5 w-3.5" />
            Back to Workspace
          </Link>
        </div>

        <section className="rounded-2xl border border-slate-200 bg-white p-6 shadow-soft dark:border-slate-800 dark:bg-slate-900">
          <div className="mb-4 flex items-center gap-2.5">
            <UserCheck className="h-4 w-4 text-blue-600 dark:text-blue-400" />
            <h2 className="text-base font-bold text-slate-900 dark:text-white">
              Account Profile
            </h2>
          </div>

          <form onSubmit={handleSaveProfile} className="space-y-4">
            <Input
              label="Display Name"
              value={name}
              onChange={(e) => setName(e.target.value)}
              error={nameError || undefined}
              maxLength={80}
              required
            />

            <Input
              label="Email Address"
              value={user.email}
              disabled
              hint={`Account created ${formatTimestamp(user.created_at)}`}
            />

            <div className="flex justify-end">
              <Button type="submit" loading={savingProfile}>
                Save Profile
              </Button>
            </div>
          </form>
        </section>

        <section className="rounded-2xl border border-slate-200 bg-white p-6 shadow-soft dark:border-slate-800 dark:bg-slate-900">
          <div className="mb-4 flex items-center justify-between">
            <div>
              <h2 className="text-base font-bold text-slate-900 dark:text-white">
                Workspace Categories
              </h2>
              <p className="text-xs text-slate-500 dark:text-slate-400">
                Manage custom labels and color tags across your tasks.
              </p>
            </div>
            <Button
              size="sm"
              onClick={() => {
                setEditingCategory(null);
                setCategoryModalOpen(true);
              }}
            >
              <FolderPlus className="h-3.5 w-3.5" />
              New Category
            </Button>
          </div>

          <div className="divide-y divide-slate-100 dark:divide-slate-800">
            {categories.map((cat) => (
              <div
                key={cat.id}
                className="flex items-center justify-between py-3 text-sm"
              >
                <div className="flex items-center gap-3">
                  <span
                    className="h-3 w-3 rounded-full"
                    style={{ backgroundColor: cat.color }}
                  />
                  <span className="font-medium text-slate-900 dark:text-slate-100">
                    {cat.name}
                  </span>
                  <span className="font-mono text-xs text-slate-400">
                    {cat.todo_count} tasks
                  </span>
                </div>
                <div className="flex items-center gap-1">
                  <Button
                    size="sm"
                    variant="ghost"
                    onClick={() => {
                      setEditingCategory(cat);
                      setCategoryModalOpen(true);
                    }}
                  >
                    <Pencil className="h-3.5 w-3.5" />
                  </Button>
                  <Button
                    size="sm"
                    variant="ghost"
                    onClick={() => void deleteCategory(cat.id)}
                  >
                    <Trash2 className="h-3.5 w-3.5 text-rose-500" />
                  </Button>
                </div>
              </div>
            ))}
          </div>
        </section>

        <section className="grid gap-4 sm:grid-cols-2">
          <div className="rounded-2xl border border-slate-200 bg-white p-5 shadow-soft dark:border-slate-800 dark:bg-slate-900">
            <div className="mb-2 flex items-center justify-between">
              <h3 className="text-sm font-bold text-slate-900 dark:text-white">
                Appearance
              </h3>
              {theme === "dark" ? (
                <Moon className="h-4 w-4 text-blue-400" />
              ) : (
                <Sun className="h-4 w-4 text-amber-500" />
              )}
            </div>
            <p className="mb-4 text-xs text-slate-500 dark:text-slate-400">
              Currently using <strong className="capitalize">{theme}</strong> mode.
            </p>
            <Button size="sm" variant="secondary" onClick={toggleTheme}>
              Toggle {theme === "dark" ? "Light" : "Dark"} Theme
            </Button>
          </div>

          <div className="rounded-2xl border border-slate-200 bg-white p-5 shadow-soft dark:border-slate-800 dark:bg-slate-900">
            <div className="mb-2 flex items-center justify-between">
              <h3 className="text-sm font-bold text-slate-900 dark:text-white">
                SP Backend Health
              </h3>
              <Activity className="h-4 w-4 text-emerald-500" />
            </div>
            {health ? (
              <div className="space-y-1 text-xs text-slate-600 dark:text-slate-300">
                <p>
                  Status:{" "}
                  <span className="font-semibold text-emerald-600 dark:text-emerald-400">
                    {health.status}
                  </span>{" "}
                  (SQLite {health.database})
                </p>
                <p className="flex items-center gap-1.5 font-mono text-[11px] text-slate-400">
                  <Server className="h-3 w-3" />
                  {API_BASE_URL}
                </p>
              </div>
            ) : (
              <p className="text-xs text-rose-500">
                {healthError || "Checking backend status..."}
              </p>
            )}
          </div>
        </section>
      </main>

      <CategoryModal
        open={categoryModalOpen}
        editingCategory={editingCategory}
        onClose={() => setCategoryModalOpen(false)}
        onCreate={createCategory}
        onUpdate={updateCategory}
      />
    </div>
  );
}
