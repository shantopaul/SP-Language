module controllers.todo_controller;

import std.web;
import std.http;
import std.json;
import config.env;
import middleware.auth;
import repositories.todo_repo;
import services.todo_service;
import util.error;

public class TodoController {
    public static fn list(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }

        let filter = new TodoFilter();
        filter.status = ctx.query("status");
        filter.priority = ctx.query("priority");
        filter.categoryId = ctx.query("category_id");
        filter.search = ctx.query("q");
        if (filter.search.trim().length() == 0) {
            filter.search = ctx.query("search");
        }
        filter.dueState = ctx.query("due");
        let sortParam = ctx.query("sort").trim();
        if (sortParam.length() > 0) {
            filter.sort = sortParam;
        }
        let orderParam = ctx.query("order").trim();
        if (orderParam.length() > 0) {
            filter.order = orderParam;
        }
        filter.page = Env.parseIntOr(ctx.query("page"), 1);
        filter.limit = Env.parseIntOr(ctx.query("limit"), 20);

        let res = TodoService.list(user.id, filter);
        return ctx.jsonStatus(res.payloadJson, 200);
    }

    public static fn stats(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }
        let res = TodoService.stats(user.id);
        return ctx.jsonStatus(res.payloadJson, 200);
    }

    public static fn getOne(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }
        let todoId = ctx.param("id");
        let res = TodoService.getById(user.id, todoId);
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
        let res = TodoService.create(user.id, body);
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
        let todoId = ctx.param("id");
        let raw = ctx.body().trim();
        if (raw.length() == 0) {
            return ErrorResponse.badRequest(ctx, "Request body is required.");
        }
        let body = Json.parse(raw);
        if (!body.isValid()) {
            body.destroy();
            return ErrorResponse.badRequest(ctx, "Invalid or malformed JSON request body.");
        }
        let res = TodoService.update(user.id, todoId, body);
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
        let todoId = ctx.param("id");
        let res = TodoService.remove(user.id, todoId);
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

    public static fn bulk(Context ctx) -> HttpResponse {
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
        let res = TodoService.bulk(user.id, body);
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
}
