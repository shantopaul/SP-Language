import type { TodoStatus } from "@/types/todo";

export function todayDateString(): string {
  const now = new Date();
  const y = now.getUTCFullYear();
  const m = String(now.getUTCMonth() + 1).padStart(2, "0");
  const d = String(now.getUTCDate()).padStart(2, "0");
  return `${y}-${m}-${d}`;
}

export function extractDatePart(iso: string | null): string | null {
  if (!iso) {
    return null;
  }
  return iso.slice(0, 10);
}

export function isOverdue(dueDate: string | null, status: TodoStatus): boolean {
  if (!dueDate || status === "done" || status === "archived") {
    return false;
  }
  const part = extractDatePart(dueDate);
  if (!part) {
    return false;
  }
  return part < todayDateString();
}

export function isDueToday(dueDate: string | null): boolean {
  const part = extractDatePart(dueDate);
  if (!part) {
    return false;
  }
  return part === todayDateString();
}

export function formatDueLabel(dueDate: string | null): string {
  const part = extractDatePart(dueDate);
  if (!part) {
    return "No due date";
  }
  const today = todayDateString();
  if (part === today) {
    return "Today";
  }
  const parsed = new Date(`${part}T00:00:00Z`);
  if (Number.isNaN(parsed.getTime())) {
    return part;
  }
  return parsed.toLocaleDateString("en-US", {
    month: "short",
    day: "numeric",
    year: "numeric",
    timeZone: "UTC",
  });
}

export function formatTimestamp(iso: string | null): string {
  if (!iso) {
    return "-";
  }
  const parsed = new Date(iso);
  if (Number.isNaN(parsed.getTime())) {
    return iso;
  }
  return parsed.toLocaleString("en-US", {
    month: "short",
    day: "numeric",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
}
