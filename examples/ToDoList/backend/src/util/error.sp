module util.error;

import std.web;
import std.http;
import util.json;

public class ErrorResponse {
    /// Build a single RFC 7807 field validation error object.
    public static fn fieldIssue(string field, string code, string message) -> string {
        return "{\"field\":" + JsonUtil.quote(field)
            + ",\"code\":" + JsonUtil.quote(code)
            + ",\"message\":" + JsonUtil.quote(message) + "}";
    }

    /// Build a JSON array string from a list of serialized field issue objects.
    public static fn fieldIssuesArray(List<string> issues) -> string {
        let out = "[";
        let i = 0;
        let count = issues.length();
        while (i < count) {
            if (i > 0) {
                out = out + ",";
            }
            out = out + issues.get(i);
            i = i + 1;
        }
        return out + "]";
    }

    /// Send an RFC 7807 application/problem+json error response.
    public static fn problem(
        Context ctx,
        int status,
        string title,
        string detail,
        string problemType = "about:blank",
        string errorsJson = "[]"
    ) -> HttpResponse {
        let reqId = ctx.header("X-Request-ID").trim();
        let body = "{\"type\":" + JsonUtil.quote(problemType)
            + ",\"title\":" + JsonUtil.quote(title)
            + ",\"status\":" + "${status}"
            + ",\"detail\":" + JsonUtil.quote(detail)
            + ",\"instance\":" + JsonUtil.quote(ctx.path());
        if (reqId.length() > 0) {
            body = body + ",\"request_id\":" + JsonUtil.quote(reqId);
        }
        body = body + ",\"errors\":" + errorsJson + "}";

        ctx.res.setStatus(status);
        ctx.res.setBody(body);
        ctx.res.setHeader("Content-Type", "application/problem+json; charset=utf-8");
        return ctx.res;
    }

    public static fn badRequest(Context ctx, string detail) -> HttpResponse {
        return ErrorResponse.problem(
            ctx,
            400,
            "Bad Request",
            detail,
            "https://todolist.splang.dev/errors/bad-request",
            "[]"
        );
    }

    public static fn unauthorized(Context ctx, string detail) -> HttpResponse {
        return ErrorResponse.problem(
            ctx,
            401,
            "Unauthorized",
            detail,
            "https://todolist.splang.dev/errors/unauthorized",
            "[]"
        );
    }

    public static fn forbidden(Context ctx, string detail) -> HttpResponse {
        return ErrorResponse.problem(
            ctx,
            403,
            "Forbidden",
            detail,
            "https://todolist.splang.dev/errors/forbidden",
            "[]"
        );
    }

    public static fn notFound(Context ctx, string detail) -> HttpResponse {
        return ErrorResponse.problem(
            ctx,
            404,
            "Not Found",
            detail,
            "https://todolist.splang.dev/errors/not-found",
            "[]"
        );
    }

    public static fn conflict(Context ctx, string detail) -> HttpResponse {
        return ErrorResponse.problem(
            ctx,
            409,
            "Conflict",
            detail,
            "https://todolist.splang.dev/errors/conflict",
            "[]"
        );
    }

    public static fn validationError(Context ctx, string detail, string errorsJson) -> HttpResponse {
        return ErrorResponse.problem(
            ctx,
            422,
            "Validation Failed",
            detail,
            "https://todolist.splang.dev/errors/validation",
            errorsJson
        );
    }

    public static fn internalError(
        Context ctx,
        string detail = "An unexpected internal error occurred."
    ) -> HttpResponse {
        return ErrorResponse.problem(
            ctx,
            500,
            "Internal Server Error",
            detail,
            "https://todolist.splang.dev/errors/internal",
            "[]"
        );
    }
}
