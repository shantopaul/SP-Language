"use client";

import React, { useState } from "react";
import Link from "next/link";
import { useRouter } from "next/navigation";
import { CheckCircle2, Lock, ShieldCheck, Sparkles } from "lucide-react";
import { Button } from "@/components/ui/Button";
import { Input } from "@/components/ui/Input";
import { useAuth } from "@/lib/auth-context";
import { useToast } from "@/lib/toast-context";
import { ApiError } from "@/types/api";

interface AuthFormCardProps {
  mode: "login" | "register";
}

export function AuthFormCard({ mode }: AuthFormCardProps) {
  const router = useRouter();
  const { login, register } = useAuth();
  const { pushToast } = useToast();

  const [name, setName] = useState<string>("");
  const [email, setEmail] = useState<string>("");
  const [password, setPassword] = useState<string>("");
  const [formError, setFormError] = useState<string>("");
  const [fieldErrors, setFieldErrors] = useState<Record<string, string>>({});
  const [submitting, setSubmitting] = useState<boolean>(false);

  const isRegister = mode === "register";

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setFormError("");
    setFieldErrors({});
    setSubmitting(true);

    try {
      if (isRegister) {
        await register({
          name: name.trim(),
          email: email.trim(),
          password,
        });
        pushToast({
          title: "Workspace created",
          description: "Default Work, Personal, and Errands lists are ready.",
          variant: "success",
        });
      } else {
        await login({
          email: email.trim(),
          password,
        });
        pushToast({
          title: "Welcome back",
          variant: "success",
        });
      }
      router.replace("/");
    } catch (err) {
      if (err instanceof ApiError) {
        setFormError(err.detail);
        const mapped: Record<string, string> = {};
        for (const item of err.errors) {
          mapped[item.field] = item.message;
        }
        setFieldErrors(mapped);
      } else {
        setFormError(
          "Unable to reach the SP Language backend server. Verify it is running."
        );
      }
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="flex min-h-screen flex-col items-center justify-center bg-slate-50 px-4 py-12 dark:bg-slate-950">
      <div className="w-full max-w-md space-y-6">
        <div className="flex flex-col items-center text-center">
          <div className="mb-3 flex h-12 w-12 items-center justify-center rounded-2xl bg-blue-600 text-white shadow-soft">
            <CheckCircle2 className="h-6 w-6" />
          </div>
          <h1 className="text-2xl font-bold tracking-tight text-slate-900 dark:text-white">
            {isRegister ? "Create your TaskFlow account" : "Sign in to TaskFlow"}
          </h1>
          <p className="mt-1.5 text-sm text-slate-500 dark:text-slate-400">
            {isRegister
              ? "Fast, isolated workspaces backed by SP Language 4.0 & SQLite WAL."
              : "Pick up where you left off across all your task collections."}
          </p>
        </div>

        <div className="rounded-2xl border border-slate-200 bg-white p-6 shadow-card sm:p-8 dark:border-slate-800 dark:bg-slate-900">
          {formError && (
            <div
              role="alert"
              className="mb-5 rounded-xl border border-rose-200 bg-rose-50 px-3.5 py-2.5 text-xs font-medium text-rose-700 dark:border-rose-900/60 dark:bg-rose-950/50 dark:text-rose-300"
            >
              {formError}
            </div>
          )}

          <form onSubmit={handleSubmit} className="space-y-4">
            {isRegister && (
              <Input
                label="Full Name"
                type="text"
                autoComplete="name"
                placeholder="Jane Doe"
                value={name}
                onChange={(e) => setName(e.target.value)}
                error={fieldErrors.name}
                required
                minLength={2}
                maxLength={80}
              />
            )}

            <Input
              label="Email Address"
              type="email"
              autoComplete="email"
              placeholder="you@example.com"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              error={fieldErrors.email}
              required
            />

            <Input
              label="Password"
              type="password"
              autoComplete={isRegister ? "new-password" : "current-password"}
              placeholder={isRegister ? "Minimum 8 characters" : "Enter your password"}
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              error={fieldErrors.password}
              required
              minLength={isRegister ? 8 : 1}
              maxLength={128}
            />

            <Button type="submit" size="lg" loading={submitting} className="w-full">
              {isRegister ? "Create Account & Launch Workspace" : "Sign In"}
            </Button>
          </form>

          <div className="mt-6 border-t border-slate-100 pt-5 text-center text-sm text-slate-600 dark:border-slate-800 dark:text-slate-400">
            {isRegister ? (
              <>
                Already have an account?{" "}
                <Link
                  href="/login"
                  className="font-semibold text-blue-600 hover:text-blue-700 dark:text-blue-400"
                >
                  Sign in
                </Link>
              </>
            ) : (
              <>
                New to TaskFlow?{" "}
                <Link
                  href="/register"
                  className="font-semibold text-blue-600 hover:text-blue-700 dark:text-blue-400"
                >
                  Create an account
                </Link>
              </>
            )}
          </div>
        </div>

        <div className="flex flex-wrap items-center justify-center gap-4 text-xs text-slate-400">
          <span className="inline-flex items-center gap-1.5">
            <ShieldCheck className="h-3.5 w-3.5 text-emerald-500" />
            Argon2id Hashes
          </span>
          <span className="inline-flex items-center gap-1.5">
            <Lock className="h-3.5 w-3.5 text-blue-500" />
            JWT Bearer Auth
          </span>
          <span className="inline-flex items-center gap-1.5">
            <Sparkles className="h-3.5 w-3.5 text-amber-500" />
            Pure SP 4.0.0 API
          </span>
        </div>
      </div>
    </div>
  );
}
