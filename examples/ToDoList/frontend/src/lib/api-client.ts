import { API_BASE_URL } from "@/lib/config";
import { tokenStorage } from "@/lib/storage";
import {
  ApiError,
  type HealthResponse,
  type ProblemDetails,
} from "@/types/api";
import type {
  AuthResponse,
  LoginInput,
  ProfileUpdateInput,
  RegisterInput,
  User,
} from "@/types/auth";
import type {
  Category,
  CategoryListResponse,
  CreateCategoryInput,
  UpdateCategoryInput,
} from "@/types/category";
import type {
  BulkPayload,
  BulkResponse,
  CreateTodoInput,
  PaginatedTodos,
  Todo,
  TodoQuery,
  TodoStats,
  UpdateTodoInput,
} from "@/types/todo";

type UnauthorizedHandler = () => void;
let onUnauthorizedCallback: UnauthorizedHandler | null = null;

export function setUnauthorizedHandler(handler: UnauthorizedHandler | null): void {
  onUnauthorizedCallback = handler;
}

interface RequestOptions {
  method?: "GET" | "POST" | "PUT" | "PATCH" | "DELETE";
  body?: unknown;
  auth?: boolean;
  triggerUnauthorizedHook?: boolean;
}

async function parseErrorResponse(res: Response): Promise<ProblemDetails> {
  const fallback: ProblemDetails = {
    title: res.statusText || "Request Error",
    status: res.status,
    detail: `Request failed with status ${res.status}.`,
    errors: [],
  };
  try {
    const text = await res.text();
    if (!text) {
      return fallback;
    }
    const parsed = JSON.parse(text) as Partial<ProblemDetails>;
    return {
      type: parsed.type,
      title: parsed.title || fallback.title,
      status: typeof parsed.status === "number" ? parsed.status : res.status,
      detail: parsed.detail || fallback.detail,
      instance: parsed.instance,
      request_id: parsed.request_id,
      errors: Array.isArray(parsed.errors) ? parsed.errors : [],
    };
  } catch {
    return fallback;
  }
}

export async function apiRequest<T>(
  path: string,
  options: RequestOptions = {}
): Promise<T> {
  const {
    method = "GET",
    body,
    auth = true,
    triggerUnauthorizedHook = true,
  } = options;

  const headers: Record<string, string> = {};
  if (body !== undefined) {
    headers["Content-Type"] = "application/json";
  }

  if (auth) {
    const token = tokenStorage.get();
    if (token) {
      headers["Authorization"] = `Bearer ${token}`;
    }
  }

  const res = await fetch(`${API_BASE_URL}${path}`, {
    method,
    headers,
    body: body !== undefined ? JSON.stringify(body) : undefined,
    cache: "no-store",
  });

  if (!res.ok) {
    if (res.status === 401 && triggerUnauthorizedHook && onUnauthorizedCallback) {
      onUnauthorizedCallback();
    }
    const problem = await parseErrorResponse(res);
    throw new ApiError(problem);
  }

  const text = await res.text();
  if (!text) {
    return {} as T;
  }
  return JSON.parse(text) as T;
}

function buildTodoQueryString(query: TodoQuery): string {
  const params = new URLSearchParams();
  if (query.status && query.status !== "all") params.set("status", query.status);
  if (query.priority && query.priority !== "all") params.set("priority", query.priority);
  if (query.category_id && query.category_id !== "all") {
    params.set("category_id", query.category_id);
  }
  if (query.q && query.q.trim().length > 0) params.set("q", query.q.trim());
  if (query.due && query.due !== "all") params.set("due", query.due);
  if (query.sort) params.set("sort", query.sort);
  if (query.order) params.set("order", query.order);
  if (query.page) params.set("page", String(query.page));
  if (query.limit) params.set("limit", String(query.limit));
  const qs = params.toString();
  return qs ? `?${qs}` : "";
}

export const healthApi = {
  check(): Promise<HealthResponse> {
    return apiRequest<HealthResponse>("/api/health", {
      auth: false,
      triggerUnauthorizedHook: false,
    });
  },
};

export const authApi = {
  register(input: RegisterInput): Promise<AuthResponse> {
    return apiRequest<AuthResponse>("/api/v1/auth/register", {
      method: "POST",
      body: input,
      auth: false,
      triggerUnauthorizedHook: false,
    });
  },

  login(input: LoginInput): Promise<AuthResponse> {
    return apiRequest<AuthResponse>("/api/v1/auth/login", {
      method: "POST",
      body: input,
      auth: false,
      triggerUnauthorizedHook: false,
    });
  },

  me(): Promise<{ user: User }> {
    return apiRequest<{ user: User }>("/api/v1/auth/me", {
      method: "GET",
    });
  },

  updateProfile(input: ProfileUpdateInput): Promise<{ user: User }> {
    return apiRequest<{ user: User }>("/api/v1/auth/me", {
      method: "PATCH",
      body: input,
    });
  },
};

export const categoryApi = {
  list(): Promise<CategoryListResponse> {
    return apiRequest<CategoryListResponse>("/api/v1/categories");
  },

  create(input: CreateCategoryInput): Promise<Category> {
    return apiRequest<Category>("/api/v1/categories", {
      method: "POST",
      body: input,
    });
  },

  update(id: string, input: UpdateCategoryInput): Promise<Category> {
    return apiRequest<Category>(`/api/v1/categories/${encodeURIComponent(id)}`, {
      method: "PATCH",
      body: input,
    });
  },

  remove(id: string): Promise<{ deleted: boolean; id: string }> {
    return apiRequest<{ deleted: boolean; id: string }>(
      `/api/v1/categories/${encodeURIComponent(id)}`,
      { method: "DELETE" }
    );
  },
};

export const todoApi = {
  list(query: TodoQuery = {}): Promise<PaginatedTodos> {
    return apiRequest<PaginatedTodos>(`/api/v1/todos${buildTodoQueryString(query)}`);
  },

  stats(): Promise<TodoStats> {
    return apiRequest<TodoStats>("/api/v1/todos/stats");
  },

  getOne(id: string): Promise<Todo> {
    return apiRequest<Todo>(`/api/v1/todos/${encodeURIComponent(id)}`);
  },

  create(input: CreateTodoInput): Promise<Todo> {
    return apiRequest<Todo>("/api/v1/todos", {
      method: "POST",
      body: input,
    });
  },

  update(id: string, input: UpdateTodoInput): Promise<Todo> {
    return apiRequest<Todo>(`/api/v1/todos/${encodeURIComponent(id)}`, {
      method: "PATCH",
      body: input,
    });
  },

  remove(id: string): Promise<{ deleted: boolean; id: string }> {
    return apiRequest<{ deleted: boolean; id: string }>(
      `/api/v1/todos/${encodeURIComponent(id)}`,
      { method: "DELETE" }
    );
  },

  bulk(payload: BulkPayload): Promise<BulkResponse> {
    return apiRequest<BulkResponse>("/api/v1/todos/bulk", {
      method: "POST",
      body: payload,
    });
  },
};
