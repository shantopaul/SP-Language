module controllers.auth_controller;

import std.web;
import std.http;
import std.json;
import services.auth_service;
import middleware.auth;
import util.json;
import util.error;

public class AuthController {
    public static fn register(Context ctx) -> HttpResponse {
        let raw = ctx.body().trim();
        if (raw.length() == 0) {
            return ErrorResponse.badRequest(ctx, "Request body is required.");
        }
        let body = Json.parse(raw);
        if (!body.isValid()) {
            body.destroy();
            return ErrorResponse.badRequest(ctx, "Invalid or malformed JSON request body.");
        }

        let name = JsonUtil.getString(body, "name", "");
        let email = JsonUtil.getString(body, "email", "");
        let password = JsonUtil.getString(body, "password", "");
        body.destroy();

        let res = AuthService.register(name, email, password);
        if (!res.ok) {
            return ErrorResponse.fromStatus(
                ctx,
                res.statusCode,
                res.errorTitle,
                res.errorDetail,
                res.fieldErrorsJson
            );
        }
        return ctx.jsonStatus(res.toAuthJson(), 201);
    }

    public static fn login(Context ctx) -> HttpResponse {
        let raw = ctx.body().trim();
        if (raw.length() == 0) {
            return ErrorResponse.badRequest(ctx, "Request body is required.");
        }
        let body = Json.parse(raw);
        if (!body.isValid()) {
            body.destroy();
            return ErrorResponse.badRequest(ctx, "Invalid or malformed JSON request body.");
        }

        let email = JsonUtil.getString(body, "email", "");
        let password = JsonUtil.getString(body, "password", "");
        body.destroy();

        let res = AuthService.login(email, password);
        if (!res.ok) {
            return ErrorResponse.fromStatus(
                ctx,
                res.statusCode,
                res.errorTitle,
                res.errorDetail,
                res.fieldErrorsJson
            );
        }
        return ctx.jsonStatus(res.toAuthJson(), 200);
    }

    public static fn me(Context ctx) -> HttpResponse {
        let user = AuthGuard.requireUser(ctx);
        if (!user.found) {
            return ErrorResponse.unauthorized(ctx, "Valid Bearer authentication token is required.");
        }
        return ctx.jsonStatus("{\"user\":" + user.toPublicJson() + "}", 200);
    }

    public static fn updateProfile(Context ctx) -> HttpResponse {
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
        let name = JsonUtil.getString(body, "name", "");
        body.destroy();

        let res = AuthService.updateProfile(user.id, name);
        if (!res.ok) {
            return ErrorResponse.fromStatus(
                ctx,
                res.statusCode,
                res.errorTitle,
                res.errorDetail,
                res.fieldErrorsJson
            );
        }
        return ctx.jsonStatus("{\"user\":" + res.user.toPublicJson() + "}", 200);
    }
}
