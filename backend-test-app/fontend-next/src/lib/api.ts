// SP Lang Backend API Client
// Connects Next.js to native SP Lang HTTP service

const API_BASE = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8000";
const API_KEY = process.env.NEXT_PUBLIC_API_KEY || "sp_live_9f83b27c4a11488da128";

export interface Item {
  id: number;
  title: string;
  description: string;
  status: "pending" | "in_progress" | "completed";
  priority: "low" | "normal" | "high";
  created_at: string;
}

export interface HealthStatus {
  status: string;
  service: string;
  framework: string;
  database: string;
  total_items: number;
  uptime: string;
}

export interface CreateItemInput {
  title: string;
  description: string;
  status?: string;
  priority?: string;
}

export interface UpdateItemInput {
  title?: string;
  description?: string;
  status?: string;
  priority?: string;
}

export interface ApiResponse<T = any> {
  data?: T;
  error?: string;
  latencyMs?: number;
}

async function request<T>(endpoint: string, options: RequestInit = {}): Promise<{ data: T; latencyMs: number }> {
  const start = performance.now();
  const res = await fetch(`${API_BASE}${endpoint}`, {
    ...options,
    headers: {
      "Content-Type": "application/json",
      "X-API-Key": API_KEY,
      "Authorization": `Bearer ${API_KEY}`,
      ...(options.headers || {}),
    },
    cache: "no-store",
  });

  const latencyMs = Math.round(performance.now() - start);

  const text = await res.text();
  if (!res.ok) {
    let errMessage = `HTTP ${res.status}: ${res.statusText}`;
    try {
      const errJson = JSON.parse(text);
      if (errJson.error) errMessage = errJson.error;
    } catch {
      // ignore json parse error
    }
    throw new Error(errMessage);
  }

  let data: T;
  try {
    data = JSON.parse(text);
  } catch (err: any) {
    console.error("Invalid JSON response from server:", text);
    throw new Error(`Invalid JSON from server: ${text.slice(0, 100)}`);
  }
  return { data, latencyMs };
}

export const api = {
  getBaseUrl(): string {
    return API_BASE;
  },

  async getHealth(): Promise<{ data: HealthStatus; latencyMs: number }> {
    return request<HealthStatus>("/api/health");
  },

  async getItems(): Promise<{ data: Item[]; latencyMs: number }> {
    return request<Item[]>("/api/items");
  },

  async getItem(id: number): Promise<{ data: Item; latencyMs: number }> {
    return request<Item>(`/api/items/${id}`);
  },

  async createItem(input: CreateItemInput): Promise<{ data: Item; latencyMs: number }> {
    return request<Item>("/api/items", {
      method: "POST",
      body: JSON.stringify(input),
    });
  },

  async updateItem(id: number, input: UpdateItemInput): Promise<{ data: { id: number; message: string }; latencyMs: number }> {
    return request<{ id: number; message: string }>(`/api/items/${id}`, {
      method: "PUT",
      body: JSON.stringify(input),
    });
  },

  async deleteItem(id: number): Promise<{ data: { success: boolean; deleted_id: number; message: string }; latencyMs: number }> {
    return request<{ success: boolean; deleted_id: number; message: string }>(`/api/items/${id}`, {
      method: "DELETE",
    });
  },
};
