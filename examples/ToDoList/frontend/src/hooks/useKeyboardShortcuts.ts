"use client";

import { useEffect } from "react";

interface ShortcutConfig {
  onNewTask?: () => void;
  onFocusSearch?: () => void;
  onEscape?: () => void;
}

function isEditableElement(target: EventTarget | null): boolean {
  if (!(target instanceof HTMLElement)) {
    return false;
  }
  const tag = target.tagName.toLowerCase();
  return (
    tag === "input" ||
    tag === "textarea" ||
    tag === "select" ||
    target.isContentEditable
  );
}

export function useKeyboardShortcuts({
  onNewTask,
  onFocusSearch,
  onEscape,
}: ShortcutConfig): void {
  useEffect(() => {
    function handleKeyDown(event: KeyboardEvent): void {
      if (event.key === "Escape") {
        if (onEscape) {
          onEscape();
        }
        return;
      }

      if (event.metaKey || event.ctrlKey || event.altKey) {
        return;
      }

      if (isEditableElement(event.target)) {
        return;
      }

      if (event.key === "n" && onNewTask) {
        event.preventDefault();
        onNewTask();
      } else if (event.key === "/" && onFocusSearch) {
        event.preventDefault();
        onFocusSearch();
      }
    }

    window.addEventListener("keydown", handleKeyDown);
    return () => {
      window.removeEventListener("keydown", handleKeyDown);
    };
  }, [onEscape, onFocusSearch, onNewTask]);
}
