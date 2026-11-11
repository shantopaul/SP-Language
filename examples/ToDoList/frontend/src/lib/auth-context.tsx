"use client";

import React, {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useState,
} from "react";
import { authApi, setUnauthorizedHandler } from "@/lib/api-client";
import { tokenStorage } from "@/lib/storage";
import type { LoginInput, RegisterInput, User } from "@/types/auth";

interface AuthContextValue {
  user: User | null;
  isLoading: boolean;
  isAuthenticated: boolean;
  login: (input: LoginInput) => Promise<User>;
  register: (input: RegisterInput) => Promise<User>;
  updateName: (name: string) => Promise<User>;
  refreshUser: () => Promise<void>;
  logout: () => void;
}

const AuthContext = createContext<AuthContextValue | null>(null);

export function AuthProvider({ children }: { children: React.ReactNode }) {
  const [user, setUser] = useState<User | null>(null);
  const [isLoading, setIsLoading] = useState<boolean>(true);

  const logout = useCallback(() => {
    tokenStorage.clear();
    setUser(null);
  }, []);

  const refreshUser = useCallback(async () => {
    const token = tokenStorage.get();
    if (!token) {
      setUser(null);
      setIsLoading(false);
      return;
    }
    try {
      const data = await authApi.me();
      setUser(data.user);
    } catch {
      tokenStorage.clear();
      setUser(null);
    } finally {
      setIsLoading(false);
    }
  }, []);

  useEffect(() => {
    setUnauthorizedHandler(() => {
      tokenStorage.clear();
      setUser(null);
    });
    void refreshUser();
    return () => {
      setUnauthorizedHandler(null);
    };
  }, [refreshUser]);

  const login = useCallback(async (input: LoginInput): Promise<User> => {
    const data = await authApi.login(input);
    tokenStorage.set(data.access_token);
    setUser(data.user);
    return data.user;
  }, []);

  const register = useCallback(async (input: RegisterInput): Promise<User> => {
    const data = await authApi.register(input);
    tokenStorage.set(data.access_token);
    setUser(data.user);
    return data.user;
  }, []);

  const updateName = useCallback(async (name: string): Promise<User> => {
    const data = await authApi.updateProfile({ name });
    setUser(data.user);
    return data.user;
  }, []);

  return (
    <AuthContext.Provider
      value={{
        user,
        isLoading,
        isAuthenticated: user !== null,
        login,
        register,
        updateName,
        refreshUser,
        logout,
      }}
    >
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth(): AuthContextValue {
  const ctx = useContext(AuthContext);
  if (!ctx) {
    throw new Error("useAuth must be used inside <AuthProvider>");
  }
  return ctx;
}
