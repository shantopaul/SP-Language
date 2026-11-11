"use client";

import React from "react";
import { Archive, CheckCheck, RotateCcw, Trash2, X } from "lucide-react";
import { Button } from "@/components/ui/Button";
import type { BulkAction, TodoPriority } from "@/types/todo";

interface BulkBarProps {
  selectedCount: number;
  onRunBulk: (action: BulkAction, priority?: TodoPriority) => Promise<void>;
  onClearSelection: () => void;
}

export function BulkBar({
  selectedCount,
  onRunBulk,
  onClearSelection,
}: BulkBarProps) {
  if (selectedCount === 0) {
    return null;
  }

  return (
    <div
      role="region"
      aria-label="Bulk task actions"
      className="flex flex-wrap items-center justify-between gap-3 rounded-xl border border-blue-200 bg-blue-50/90 px-4 py-2.5 dark:border-blue-900 dark:bg-blue-950/50"
    >
      <div className="flex items-center gap-2">
        <span className="inline-flex h-6 min-w-[24px] items-center justify-center rounded-full bg-blue-600 px-2 font-mono text-xs font-bold text-white">
          {selectedCount}
        </span>
        <span className="text-sm font-medium text-blue-950 dark:text-blue-100">
          task{selectedCount === 1 ? "" : "s"} selected
        </span>
      </div>

      <div className="flex flex-wrap items-center gap-1.5">
        <Button
          size="sm"
          variant="secondary"
          onClick={() => void onRunBulk("complete")}
        >
          <CheckCheck className="h-3.5 w-3.5 text-emerald-600" />
          Mark Done
        </Button>

        <Button
          size="sm"
          variant="secondary"
          onClick={() => void onRunBulk("reopen")}
        >
          <RotateCcw className="h-3.5 w-3.5 text-blue-600" />
          Reopen
        </Button>

        <Button
          size="sm"
          variant="secondary"
          onClick={() => void onRunBulk("archive")}
        >
          <Archive className="h-3.5 w-3.5 text-slate-500" />
          Archive
        </Button>

        <Button
          size="sm"
          variant="secondary"
          onClick={() => void onRunBulk("set_priority", "urgent")}
        >
          Flag Urgent
        </Button>

        <Button
          size="sm"
          variant="danger"
          onClick={() => void onRunBulk("delete")}
        >
          <Trash2 className="h-3.5 w-3.5" />
          Delete
        </Button>

        <button
          type="button"
          onClick={onClearSelection}
          aria-label="Clear selection"
          className="ml-1 rounded-lg p-1.5 text-blue-700 hover:bg-blue-100 dark:text-blue-300 dark:hover:bg-blue-900/60"
        >
          <X className="h-4 w-4" />
        </button>
      </div>
    </div>
  );
}
