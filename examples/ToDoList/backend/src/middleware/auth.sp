module middleware.auth;

import std.web;
import models.user;
import services.auth_service;

public class AuthGuard {
    /// Extract Bearer token from Authorization header and resolve the authenticated user.
    public static fn requireUser(Context ctx) -> User {
        let authHeader = ctx.header("Authorization").trim();
        if (authHeader.length() == 0) {
            authHeader = ctx.header("authorization").trim();
        }
        if (authHeader.length() == 0) {
            return new User();
        }
        return AuthService.verifyBearerHeader(authHeader);
    }
}
