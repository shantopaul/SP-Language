export type TodoStatus = "todo" | "in_progress" | "done" | "archived";
export type TodoPriority = "low" | "medium" | "high" | "urgent";
export type TodoSortField =
  | "created_at"
  | "due_date"
  | "priority"
  | "updated_at"
  | "title"
  | "sort_order";
export type SortOrder = "asc" | "desc";
export type TodoDueFilter = "all" | "overdue" | "today" | "upcoming" | "none";

export interface TodoCategorySummary {
  id: string;
  name: string;
  color: string;
}

export interface Todo {
  id: string;
  user_id: string;
  category_id: string | null;
  category: TodoCategorySummary | null;
  title: string;
  description: string;
  status: TodoStatus;
  priority: TodoPriority;
  due_date: string | null;
  completed_at: string | null;
  sort_order: number;
  created_at: string;
  updated_at: string;
}

export interface TodoStats {
  total: number;
  todo: number;
  in_progress: number;
  done: number;
  archived: number;
  overdue: number;
  due_today: number;
  urgent: number;
  high: number;
}

export interface PaginationMeta {
  total: number;
  page: number;
  limit: number;
  total_pages: number;
  has_next: boolean;
  has_prev: boolean;
}

export interface PaginatedTodos {
  items: Todo[];
  meta: PaginationMeta;
}

export interface TodoQuery {
  status?: "all" | "active" | TodoStatus;
  priority?: "all" | TodoPriority;
  category_id?: string;
  q?: string;
  due?: TodoDueFilter;
  sort?: TodoSortField;
  order?: SortOrder;
  page?: number;
  limit?: number;
}

export interface CreateTodoInput {
  title: string;
  description?: string;
  status?: TodoStatus;
  priority?: TodoPriority;
  category_id?: string | null;
  due_date?: string | null;
  sort_order?: number;
}

export interface UpdateTodoInput {
  title?: string;
  description?: string;
  status?: TodoStatus;
  priority?: TodoPriority;
  category_id?: string | null;
  due_date?: string | null;
  sort_order?: number;
}

export type BulkAction =
  | "complete"
  | "reopen"
  | "archive"
  | "delete"
  | "set_priority";

export interface BulkPayload {
  action: BulkAction;
  ids: string[];
  priority?: TodoPriority;
}

export interface BulkResponse {
  action: BulkAction;
  affected: number;
  requested: number;
}
