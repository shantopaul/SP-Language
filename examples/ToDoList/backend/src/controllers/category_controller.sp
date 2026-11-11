module controllers.category_controller;

import std.web;
import std.http;
import std.json;
import middleware.auth;
import services.category_service;
import util.error;

public class CategoryController {
    public static fn list(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }
        let res = CategoryService.listForUser(user.id);
        return ctx.jsonStatus(res.payloadJson, 200);
    }

    public static fn create(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }
        let raw = ctx.body().trim();
        if (raw.length() == 0) {
            return ErrorResponse.badRequest(ctx, "Request body is required.");
        }
        let body = Json.parse(raw);
        if (!body.isValid()) {
            body.destroy();
            return ErrorResponse.badRequest(ctx, "Invalid or malformed JSON request body.");
        }
        let res = CategoryService.create(user.id, body);
        body.destroy();

        if (!res.ok) {
            return ErrorResponse.fromStatus(
                ctx,
                res.statusCode,
                res.errorTitle,
                res.errorDetail,
                res.fieldErrorsJson
            );
        }
        return ctx.jsonStatus(res.payloadJson, 201);
    }

    public static fn update(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }
        let categoryId = ctx.param("id");
        let raw = ctx.body().trim();
        if (raw.length() == 0) {
            return ErrorResponse.badRequest(ctx, "Request body is required.");
        }
        let body = Json.parse(raw);
        if (!body.isValid()) {
            body.destroy();
            return ErrorResponse.badRequest(ctx, "Invalid or malformed JSON request body.");
        }
        let res = CategoryService.update(user.id, categoryId, body);
        body.destroy();

        if (!res.ok) {
            return ErrorResponse.fromStatus(
                ctx,
                res.statusCode,
                res.errorTitle,
                res.errorDetail,
                res.fieldErrorsJson
            );
        }
        return ctx.jsonStatus(res.payloadJson, 200);
    }

    public static fn remove(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }
        let categoryId = ctx.param("id");
        let res = CategoryService.remove(user.id, categoryId);
        if (!res.ok) {
            return ErrorResponse.fromStatus(
                ctx,
                res.statusCode,
                res.errorTitle,
                res.errorDetail,
                res.fieldErrorsJson
            );
        }
        return ctx.jsonStatus(res.payloadJson, 200);
    }
}
