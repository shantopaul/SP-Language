export interface Category {
  id: string;
  user_id: string;
  name: string;
  color: string;
  sort_order: number;
  todo_count: number;
  created_at: string;
  updated_at: string;
}

export interface CategoryListResponse {
  items: Category[];
  total: number;
}

export interface CreateCategoryInput {
  name: string;
  color?: string;
  sort_order?: number;
}

export interface UpdateCategoryInput {
  name?: string;
  color?: string;
  sort_order?: number;
}

export const PRESET_CATEGORY_COLORS: readonly string[] = [
  "#3B82F6",
  "#10B981",
  "#F59E0B",
  "#EF4444",
  "#8B5CF6",
  "#EC4899",
  "#06B6D4",
  "#64748B",
];
