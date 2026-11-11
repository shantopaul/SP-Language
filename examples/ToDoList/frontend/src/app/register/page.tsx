"use client";

import React, { useEffect } from "react";
import { useRouter } from "next/navigation";
import { AuthFormCard } from "@/components/auth/AuthFormCard";
import { useAuth } from "@/lib/auth-context";

export default function RegisterPage() {
  const router = useRouter();
  const { isAuthenticated, isLoading } = useAuth();

  useEffect(() => {
    if (!isLoading && isAuthenticated) {
      router.replace("/");
    }
  }, [isAuthenticated, isLoading, router]);

  return <AuthFormCard mode="register" />;
}
