module services.category_service;

import std.crypto;
import std.json;
import models.category;
import repositories.category_repo;
import util.json;
import util.validation;
import util.error;

public class CategoryResult {
    public bool ok = false;
    public int statusCode = 200;
    public string errorTitle = "";
    public string errorDetail = "";
    public string fieldErrorsJson = "[]";
    public string payloadJson = "{}";
    public Category category = new Category();
}

public class CategoryService {
    static fn makeError(
        int statusCode,
        string title,
        string detail,
        string fieldErrorsJson = "[]"
    ) -> CategoryResult {
        let res = new CategoryResult();
        res.ok = false;
        res.statusCode = statusCode;
        res.errorTitle = title;
        res.errorDetail = detail;
        res.fieldErrorsJson = fieldErrorsJson;
        return res;
    }

    public static fn listForUser(string userId) -> CategoryResult {
        let cats = CategoryRepository.listByUser(userId);
        let arr = "[";
        let i = 0;
        let count = cats.length();
        while (i < count) {
            if (i > 0) {
                arr = arr + ",";
            }
            arr = arr + cats.get(i).toJson();
            i = i + 1;
        }
        arr = arr + "]";

        let res = new CategoryResult();
        res.ok = true;
        res.statusCode = 200;
        res.payloadJson = "{\"items\":" + arr + ",\"total\":${count}}";
        return res;
    }

    public static fn create(string userId, JsonValue body) -> CategoryResult {
        let name = JsonUtil.getString(body, "name", "").trim();
        let color = JsonUtil.getString(body, "color", "#3B82F6").trim();
        if (color.length() == 0) {
            color = "#3B82F6";
        }
        let sortOrder = JsonUtil.getInt(body, "sort_order", 0);

        let issues = new List<string>();
        if (name.length() < 1 || name.length() > 40) {
            issues.push(ErrorResponse.fieldIssue(
                "name",
                "INVALID_LENGTH",
                "Category name must be between 1 and 40 characters."
            ));
        }
        if (!ValidationUtil.isValidHexColor(color)) {
            issues.push(ErrorResponse.fieldIssue(
                "color",
                "INVALID_COLOR",
                "Color must be a 6-digit hex code such as #3B82F6."
            ));
        }
        if (sortOrder < 0 || sortOrder > 100000) {
            issues.push(ErrorResponse.fieldIssue(
                "sort_order",
                "OUT_OF_RANGE",
                "sort_order must be between 0 and 100000."
            ));
        }
        if (issues.length() > 0) {
            return CategoryService.makeError(
                422,
                "Validation Failed",
                "Category input failed validation.",
                ErrorResponse.fieldIssuesArray(issues)
            );
        }

        if (CategoryRepository.existsNameForUser(userId, name, "")) {
            return CategoryService.makeError(
                409,
                "Conflict",
                "A category with this name already exists."
            );
        }

        let created = CategoryRepository.create(Crypto.uuid(), userId, name, color, sortOrder);
        if (!created.found) {
            return CategoryService.makeError(
                500,
                "Internal Server Error",
                "Could not create category."
            );
        }

        let res = new CategoryResult();
        res.ok = true;
        res.statusCode = 201;
        res.category = created;
        res.payloadJson = created.toJson();
        return res;
    }

    public static fn update(string userId, string categoryId, JsonValue body) -> CategoryResult {
        if (!ValidationUtil.isValidUuid(categoryId)) {
            return CategoryService.makeError(400, "Bad Request", "Invalid category ID format.");
        }
        let existing = CategoryRepository.findById(categoryId);
        if (!existing.found) {
            return CategoryService.makeError(404, "Not Found", "Category not found.");
        }
        if (existing.userId != userId) {
            return CategoryService.makeError(403, "Forbidden", "You do not have access to this category.");
        }

        let nextName = existing.name;
        if (JsonUtil.hasField(body, "name") && !JsonUtil.isNullField(body, "name")) {
            nextName = JsonUtil.getString(body, "name", "").trim();
        }
        let nextColor = existing.color;
        if (JsonUtil.hasField(body, "color") && !JsonUtil.isNullField(body, "color")) {
            nextColor = JsonUtil.getString(body, "color", "").trim();
        }
        let nextSortOrder = existing.sortOrder;
        if (JsonUtil.hasField(body, "sort_order") && !JsonUtil.isNullField(body, "sort_order")) {
            nextSortOrder = JsonUtil.getInt(body, "sort_order", existing.sortOrder);
        }

        let issues = new List<string>();
        if (nextName.length() < 1 || nextName.length() > 40) {
            issues.push(ErrorResponse.fieldIssue(
                "name",
                "INVALID_LENGTH",
                "Category name must be between 1 and 40 characters."
            ));
        }
        if (!ValidationUtil.isValidHexColor(nextColor)) {
            issues.push(ErrorResponse.fieldIssue(
                "color",
                "INVALID_COLOR",
                "Color must be a 6-digit hex code such as #3B82F6."
            ));
        }
        if (nextSortOrder < 0 || nextSortOrder > 100000) {
            issues.push(ErrorResponse.fieldIssue(
                "sort_order",
                "OUT_OF_RANGE",
                "sort_order must be between 0 and 100000."
            ));
        }
        if (issues.length() > 0) {
            return CategoryService.makeError(
                422,
                "Validation Failed",
                "Category update failed validation.",
                ErrorResponse.fieldIssuesArray(issues)
            );
        }

        if (CategoryRepository.existsNameForUser(userId, nextName, categoryId)) {
            return CategoryService.makeError(
                409,
                "Conflict",
                "Another category with this name already exists."
            );
        }

        let updated = CategoryRepository.update(
            categoryId,
            userId,
            nextName,
            nextColor,
            nextSortOrder
        );
        if (!updated.found) {
            return CategoryService.makeError(
                500,
                "Internal Server Error",
                "Could not update category."
            );
        }

        let res = new CategoryResult();
        res.ok = true;
        res.statusCode = 200;
        res.category = updated;
        res.payloadJson = updated.toJson();
        return res;
    }

    public static fn remove(string userId, string categoryId) -> CategoryResult {
        if (!ValidationUtil.isValidUuid(categoryId)) {
            return CategoryService.makeError(400, "Bad Request", "Invalid category ID format.");
        }
        let existing = CategoryRepository.findById(categoryId);
        if (!existing.found) {
            return CategoryService.makeError(404, "Not Found", "Category not found.");
        }
        if (existing.userId != userId) {
            return CategoryService.makeError(403, "Forbidden", "You do not have access to this category.");
        }
        let deleted = CategoryRepository.deleteById(categoryId, userId);
        if (!deleted) {
            return CategoryService.makeError(
                500,
                "Internal Server Error",
                "Could not delete category."
            );
        }
        let res = new CategoryResult();
        res.ok = true;
        res.statusCode = 200;
        res.payloadJson = "{\"deleted\":true,\"id\":" + JsonUtil.quote(categoryId) + "}";
        return res;
    }
}
