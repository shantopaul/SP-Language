module services.todo_service;

import std.crypto;
import std.json;
import db.schema;
import models.todo;
import models.category;
import repositories.todo_repo;
import repositories.category_repo;
import util.json;
import util.validation;
import util.error;

public class TodoResult {
    public bool ok = false;
    public int statusCode = 200;
    public string errorTitle = "";
    public string errorDetail = "";
    public string fieldErrorsJson = "[]";
    public string payloadJson = "{}";
    public Todo todo = new Todo();
}

public class TodoService {
    static fn makeError(
        int statusCode,
        string title,
        string detail,
        string fieldErrorsJson = "[]"
    ) -> TodoResult {
        let res = new TodoResult();
        res.ok = false;
        res.statusCode = statusCode;
        res.errorTitle = title;
        res.errorDetail = detail;
        res.fieldErrorsJson = fieldErrorsJson;
        return res;
    }

    static fn currentTimestamp() -> string {
        let db = DatabaseManager.open();
        let ts = DatabaseManager.utcNow(db);
        if (db.isOpen()) {
            db.close();
        }
        return ts;
    }

    public static fn list(string userId, TodoFilter filter) -> TodoResult {
        let page = TodoRepository.listByUser(userId, filter);
        let res = new TodoResult();
        res.ok = true;
        res.statusCode = 200;
        res.payloadJson = page.toJson();
        return res;
    }

    public static fn getById(string userId, string todoId) -> TodoResult {
        if (!ValidationUtil.isValidUuid(todoId)) {
            return TodoService.makeError(400, "Bad Request", "Invalid todo ID format.");
        }
        let todo = TodoRepository.findById(todoId);
        if (!todo.found) {
            return TodoService.makeError(404, "Not Found", "Todo item not found.");
        }
        if (todo.userId != userId) {
            return TodoService.makeError(403, "Forbidden", "You do not have access to this todo.");
        }
        let res = new TodoResult();
        res.ok = true;
        res.statusCode = 200;
        res.todo = todo;
        res.payloadJson = todo.toJson();
        return res;
    }

    public static fn create(string userId, JsonValue body) -> TodoResult {
        let title = JsonUtil.getString(body, "title", "").trim();
        let description = JsonUtil.getString(body, "description", "");
        let status = JsonUtil.getString(body, "status", "todo").trim().lower();
        if (status.length() == 0) {
            status = "todo";
        }
        let priority = JsonUtil.getString(body, "priority", "medium").trim().lower();
        if (priority.length() == 0) {
            priority = "medium";
        }
        let categoryId = JsonUtil.getString(body, "category_id", "").trim();
        let dueDate = JsonUtil.getString(body, "due_date", "").trim();
        let sortOrder = JsonUtil.getInt(body, "sort_order", 0);

        let issues = new List<string>();
        if (title.length() < 1 || title.length() > 200) {
            issues.push(ErrorResponse.fieldIssue(
                "title",
                "INVALID_LENGTH",
                "Title must be between 1 and 200 characters."
            ));
        }
        if (description.length() > 4000) {
            issues.push(ErrorResponse.fieldIssue(
                "description",
                "TOO_LONG",
                "Description cannot exceed 4000 characters."
            ));
        }
        if (!ValidationUtil.isValidStatus(status)) {
            issues.push(ErrorResponse.fieldIssue(
                "status",
                "INVALID_ENUM",
                "Status must be one of: todo, in_progress, done, archived."
            ));
        }
        if (!ValidationUtil.isValidPriority(priority)) {
            issues.push(ErrorResponse.fieldIssue(
                "priority",
                "INVALID_ENUM",
                "Priority must be one of: low, medium, high, urgent."
            ));
        }
        if (dueDate.length() > 0 && !ValidationUtil.isValidIsoDate(dueDate)) {
            issues.push(ErrorResponse.fieldIssue(
                "due_date",
                "INVALID_DATE",
                "due_date must be an ISO-8601 date (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SSZ)."
            ));
        }
        if (categoryId.length() > 0 && !ValidationUtil.isValidUuid(categoryId)) {
            issues.push(ErrorResponse.fieldIssue(
                "category_id",
                "INVALID_UUID",
                "category_id must be a valid UUID."
            ));
        }
        if (sortOrder < 0 || sortOrder > 1000000) {
            issues.push(ErrorResponse.fieldIssue(
                "sort_order",
                "OUT_OF_RANGE",
                "sort_order must be between 0 and 1000000."
            ));
        }
        if (issues.length() > 0) {
            return TodoService.makeError(
                422,
                "Validation Failed",
                "Todo input failed validation.",
                ErrorResponse.fieldIssuesArray(issues)
            );
        }

        if (categoryId.length() > 0) {
            let cat = CategoryRepository.findById(categoryId);
            if (!cat.found || cat.userId != userId) {
                return TodoService.makeError(
                    422,
                    "Validation Failed",
                    "Referenced category does not exist for this user.",
                    "[" + ErrorResponse.fieldIssue(
                        "category_id",
                        "CATEGORY_NOT_FOUND",
                        "Category not found in your workspace."
                    ) + "]"
                );
            }
        }

        let completedAt = "";
        if (status == "done") {
            completedAt = TodoService.currentTimestamp();
        }

        let created = TodoRepository.create(
            Crypto.uuid(),
            userId,
            categoryId,
            title,
            description,
            status,
            priority,
            dueDate,
            completedAt,
            sortOrder
        );
        if (!created.found) {
            return TodoService.makeError(500, "Internal Server Error", "Could not create todo.");
        }

        let res = new TodoResult();
        res.ok = true;
        res.statusCode = 201;
        res.todo = created;
        res.payloadJson = created.toJson();
        return res;
    }

    public static fn update(string userId, string todoId, JsonValue body) -> TodoResult {
        if (!ValidationUtil.isValidUuid(todoId)) {
            return TodoService.makeError(400, "Bad Request", "Invalid todo ID format.");
        }
        let existing = TodoRepository.findById(todoId);
        if (!existing.found) {
            return TodoService.makeError(404, "Not Found", "Todo item not found.");
        }
        if (existing.userId != userId) {
            return TodoService.makeError(403, "Forbidden", "You do not have access to this todo.");
        }

        let nextTitle = existing.title;
        if (JsonUtil.hasField(body, "title")) {
            if (JsonUtil.isNullField(body, "title")) {
                nextTitle = "";
            } else {
                nextTitle = JsonUtil.getString(body, "title", "").trim();
            }
        }

        let nextDesc = existing.description;
        if (JsonUtil.hasField(body, "description")) {
            if (JsonUtil.isNullField(body, "description")) {
                nextDesc = "";
            } else {
                nextDesc = JsonUtil.getString(body, "description", "");
            }
        }

        let nextStatus = existing.status;
        if (JsonUtil.hasField(body, "status") && !JsonUtil.isNullField(body, "status")) {
            nextStatus = JsonUtil.getString(body, "status", "").trim().lower();
        }

        let nextPriority = existing.priority;
        if (JsonUtil.hasField(body, "priority") && !JsonUtil.isNullField(body, "priority")) {
            nextPriority = JsonUtil.getString(body, "priority", "").trim().lower();
        }

        let nextCategoryId = existing.categoryId;
        if (JsonUtil.hasField(body, "category_id")) {
            if (JsonUtil.isNullField(body, "category_id")) {
                nextCategoryId = "";
            } else {
                nextCategoryId = JsonUtil.getString(body, "category_id", "").trim();
            }
        }

        let nextDueDate = existing.dueDate;
        if (JsonUtil.hasField(body, "due_date")) {
            if (JsonUtil.isNullField(body, "due_date")) {
                nextDueDate = "";
            } else {
                nextDueDate = JsonUtil.getString(body, "due_date", "").trim();
            }
        }

        let nextSortOrder = existing.sortOrder;
        if (JsonUtil.hasField(body, "sort_order") && !JsonUtil.isNullField(body, "sort_order")) {
            nextSortOrder = JsonUtil.getInt(body, "sort_order", existing.sortOrder);
        }

        let issues = new List<string>();
        if (nextTitle.length() < 1 || nextTitle.length() > 200) {
            issues.push(ErrorResponse.fieldIssue(
                "title",
                "INVALID_LENGTH",
                "Title must be between 1 and 200 characters."
            ));
        }
        if (nextDesc.length() > 4000) {
            issues.push(ErrorResponse.fieldIssue(
                "description",
                "TOO_LONG",
                "Description cannot exceed 4000 characters."
            ));
        }
        if (!ValidationUtil.isValidStatus(nextStatus)) {
            issues.push(ErrorResponse.fieldIssue(
                "status",
                "INVALID_ENUM",
                "Status must be one of: todo, in_progress, done, archived."
            ));
        }
        if (!ValidationUtil.isValidPriority(nextPriority)) {
            issues.push(ErrorResponse.fieldIssue(
                "priority",
                "INVALID_ENUM",
                "Priority must be one of: low, medium, high, urgent."
            ));
        }
        if (nextDueDate.length() > 0 && !ValidationUtil.isValidIsoDate(nextDueDate)) {
            issues.push(ErrorResponse.fieldIssue(
                "due_date",
                "INVALID_DATE",
                "due_date must be an ISO-8601 date (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SSZ)."
            ));
        }
        if (nextCategoryId.length() > 0 && !ValidationUtil.isValidUuid(nextCategoryId)) {
            issues.push(ErrorResponse.fieldIssue(
                "category_id",
                "INVALID_UUID",
                "category_id must be a valid UUID."
            ));
        }
        if (nextSortOrder < 0 || nextSortOrder > 1000000) {
            issues.push(ErrorResponse.fieldIssue(
                "sort_order",
                "OUT_OF_RANGE",
                "sort_order must be between 0 and 1000000."
            ));
        }
        if (issues.length() > 0) {
            return TodoService.makeError(
                422,
                "Validation Failed",
                "Todo update failed validation.",
                ErrorResponse.fieldIssuesArray(issues)
            );
        }

        if (nextCategoryId.length() > 0 && nextCategoryId != existing.categoryId) {
            let cat = CategoryRepository.findById(nextCategoryId);
            if (!cat.found || cat.userId != userId) {
                return TodoService.makeError(
                    422,
                    "Validation Failed",
                    "Referenced category does not exist for this user.",
                    "[" + ErrorResponse.fieldIssue(
                        "category_id",
                        "CATEGORY_NOT_FOUND",
                        "Category not found in your workspace."
                    ) + "]"
                );
            }
        }

        let nextCompletedAt = existing.completedAt;
        if (nextStatus == "done") {
            if (nextCompletedAt.length() == 0) {
                nextCompletedAt = TodoService.currentTimestamp();
            }
        } else if (nextStatus == "todo" || nextStatus == "in_progress") {
            nextCompletedAt = "";
        }

        let updated = TodoRepository.update(
            todoId,
            userId,
            nextCategoryId,
            nextTitle,
            nextDesc,
            nextStatus,
            nextPriority,
            nextDueDate,
            nextCompletedAt,
            nextSortOrder
        );
        if (!updated.found) {
            return TodoService.makeError(500, "Internal Server Error", "Could not update todo.");
        }

        let res = new TodoResult();
        res.ok = true;
        res.statusCode = 200;
        res.todo = updated;
        res.payloadJson = updated.toJson();
        return res;
    }

    public static fn remove(string userId, string todoId) -> TodoResult {
        if (!ValidationUtil.isValidUuid(todoId)) {
            return TodoService.makeError(400, "Bad Request", "Invalid todo ID format.");
        }
        let existing = TodoRepository.findById(todoId);
        if (!existing.found) {
            return TodoService.makeError(404, "Not Found", "Todo item not found.");
        }
        if (existing.userId != userId) {
            return TodoService.makeError(403, "Forbidden", "You do not have access to this todo.");
        }
        let deleted = TodoRepository.deleteById(todoId, userId);
        if (!deleted) {
            return TodoService.makeError(500, "Internal Server Error", "Could not delete todo.");
        }
        let res = new TodoResult();
        res.ok = true;
        res.statusCode = 200;
        res.payloadJson = "{\"deleted\":true,\"id\":" + JsonUtil.quote(todoId) + "}";
        return res;
    }

    public static fn stats(string userId) -> TodoResult {
        let s = TodoRepository.getStats(userId);
        let res = new TodoResult();
        res.ok = true;
        res.statusCode = 200;
        res.payloadJson = s.toJson();
        return res;
    }

    public static fn bulk(string userId, JsonValue body) -> TodoResult {
        let action = JsonUtil.getString(body, "action", "").trim().lower();
        let priority = JsonUtil.getString(body, "priority", "").trim().lower();
        let ids = JsonUtil.getStringArray(body, "ids");

        let issues = new List<string>();
        if (!ValidationUtil.isValidBulkAction(action)) {
            issues.push(ErrorResponse.fieldIssue(
                "action",
                "INVALID_ENUM",
                "action must be one of: complete, reopen, archive, delete, set_priority."
            ));
        }
        if (ids.length() < 1 || ids.length() > 100) {
            issues.push(ErrorResponse.fieldIssue(
                "ids",
                "INVALID_SIZE",
                "ids must contain between 1 and 100 todo IDs."
            ));
        } else {
            let i = 0;
            while (i < ids.length()) {
                if (!ValidationUtil.isValidUuid(ids.get(i))) {
                    issues.push(ErrorResponse.fieldIssue(
                        "ids",
                        "INVALID_UUID",
                        "All items in ids must be valid UUIDs."
                    ));
                    break;
                }
                i = i + 1;
            }
        }
        if (action == "set_priority" && !ValidationUtil.isValidPriority(priority)) {
            issues.push(ErrorResponse.fieldIssue(
                "priority",
                "INVALID_ENUM",
                "priority is required for set_priority and must be low, medium, high, or urgent."
            ));
        }
        if (issues.length() > 0) {
            return TodoService.makeError(
                422,
                "Validation Failed",
                "Bulk action request failed validation.",
                ErrorResponse.fieldIssuesArray(issues)
            );
        }

        let affected = TodoRepository.bulkExecute(userId, ids, action, priority);
        let res = new TodoResult();
        res.ok = true;
        res.statusCode = 200;
        res.payloadJson = "{\"action\":" + JsonUtil.quote(action)
            + ",\"affected\":${affected}"
            + ",\"requested\":${ids.length()}"
            + "}";
        return res;
    }
}
