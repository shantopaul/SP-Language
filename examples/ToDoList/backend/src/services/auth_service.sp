module services.auth_service;

import std.crypto;
import std.crypto.password;
import std.security.jwt;
import config.env;
import models.user;
import repositories.user_repo;
import repositories.category_repo;
import util.json;
import util.validation;
import util.error;
import util.logger;

public class AuthResult {
    public bool ok = false;
    public int statusCode = 200;
    public string errorTitle = "";
    public string errorDetail = "";
    public string fieldErrorsJson = "[]";
    public string token = "";
    public int expiresIn = 86400;
    public User user = new User();

    public fn toAuthJson() -> string {
        return "{\"access_token\":" + JsonUtil.quote(this.token)
            + ",\"token_type\":\"Bearer\""
            + ",\"expires_in\":${this.expiresIn}"
            + ",\"user\":" + this.user.toPublicJson()
            + "}";
    }
}

public class AuthService {
    static fn makeError(
        int statusCode,
        string title,
        string detail,
        string fieldErrorsJson = "[]"
    ) -> AuthResult {
        let res = new AuthResult();
        res.ok = false;
        res.statusCode = statusCode;
        res.errorTitle = title;
        res.errorDetail = detail;
        res.fieldErrorsJson = fieldErrorsJson;
        return res;
    }

    static fn signForUser(User user) -> string {
        let claims = "{\"sub\":" + JsonUtil.quote(user.id)
            + ",\"email\":" + JsonUtil.quote(user.email)
            + ",\"name\":" + JsonUtil.quote(user.name) + "}";
        return Jwt.signExp(claims, Env.jwtSecret(), Env.jwtTtlSeconds());
    }

    public static fn register(string name, string email, string password) -> AuthResult {
        let cleanName = name.trim();
        let cleanEmail = email.trim().lower();
        let issues = new List<string>();

        if (cleanName.length() < 2 || cleanName.length() > 80) {
            issues.push(ErrorResponse.fieldIssue(
                "name",
                "INVALID_LENGTH",
                "Name must be between 2 and 80 characters."
            ));
        }
        if (!ValidationUtil.isValidEmail(cleanEmail)) {
            issues.push(ErrorResponse.fieldIssue(
                "email",
                "INVALID_EMAIL",
                "Please provide a valid email address."
            ));
        }
        if (password.length() < 8 || password.length() > 128) {
            issues.push(ErrorResponse.fieldIssue(
                "password",
                "INVALID_LENGTH",
                "Password must be between 8 and 128 characters."
            ));
        }
        if (issues.length() > 0) {
            return AuthService.makeError(
                422,
                "Validation Failed",
                "Registration input failed validation.",
                ErrorResponse.fieldIssuesArray(issues)
            );
        }

        if (!Env.hasValidJwtSecret()) {
            AppLog.error("auth_service", "JWT_SECRET is missing or shorter than 32 characters.");
            return AuthService.makeError(
                500,
                "Internal Server Error",
                "Authentication configuration is unavailable."
            );
        }

        let existing = UserRepository.findByEmail(cleanEmail);
        if (existing.found) {
            return AuthService.makeError(
                409,
                "Conflict",
                "An account with this email already exists."
            );
        }

        let passwordHash = Password.hash(password);
        if (passwordHash.length() == 0) {
            AppLog.error("auth_service", "Password.hash returned empty hash during registration.");
            return AuthService.makeError(
                500,
                "Internal Server Error",
                "Could not process credentials."
            );
        }

        let userId = Crypto.uuid();
        let created = UserRepository.createUser(userId, cleanEmail, cleanName, passwordHash);
        if (!created.found) {
            return AuthService.makeError(
                500,
                "Internal Server Error",
                "Could not persist user account."
            );
        }

        CategoryRepository.seedDefaults(created.id);

        let token = AuthService.signForUser(created);
        if (token.length() == 0) {
            AppLog.error("auth_service", "Jwt.signExp returned empty token during registration.");
            return AuthService.makeError(
                500,
                "Internal Server Error",
                "Could not issue access token."
            );
        }

        let res = new AuthResult();
        res.ok = true;
        res.statusCode = 201;
        res.token = token;
        res.expiresIn = Env.jwtTtlSeconds();
        res.user = created;
        return res;
    }

    public static fn login(string email, string password) -> AuthResult {
        let cleanEmail = email.trim().lower();
        let issues = new List<string>();
        if (cleanEmail.length() == 0) {
            issues.push(ErrorResponse.fieldIssue("email", "REQUIRED", "Email is required."));
        }
        if (password.length() == 0) {
            issues.push(ErrorResponse.fieldIssue("password", "REQUIRED", "Password is required."));
        }
        if (issues.length() > 0) {
            return AuthService.makeError(
                422,
                "Validation Failed",
                "Login credentials are required.",
                ErrorResponse.fieldIssuesArray(issues)
            );
        }

        if (!Env.hasValidJwtSecret()) {
            AppLog.error("auth_service", "JWT_SECRET is missing or shorter than 32 characters.");
            return AuthService.makeError(
                500,
                "Internal Server Error",
                "Authentication configuration is unavailable."
            );
        }

        let user = UserRepository.findByEmail(cleanEmail);
        if (!user.found || !Password.verify(password, user.passwordHash)) {
            return AuthService.makeError(401, "Unauthorized", "Invalid email or password.");
        }

        let token = AuthService.signForUser(user);
        if (token.length() == 0) {
            AppLog.error("auth_service", "Jwt.signExp returned empty token during login.");
            return AuthService.makeError(
                500,
                "Internal Server Error",
                "Could not issue access token."
            );
        }

        let res = new AuthResult();
        res.ok = true;
        res.statusCode = 200;
        res.token = token;
        res.expiresIn = Env.jwtTtlSeconds();
        res.user = user;
        return res;
    }

    public static fn verifyBearerHeader(string authHeader) -> User {
        let empty = new User();
        if (!Env.hasValidJwtSecret()) {
            return empty;
        }
        let header = authHeader.trim();
        if (header.length() < 8) {
            return empty;
        }
        if (!header.lower().startsWith("bearer ")) {
            return empty;
        }
        let token = header.substring(7, header.length()).trim();
        if (token.length() == 0) {
            return empty;
        }
        let vr = Jwt.verify(token, Env.jwtSecret());
        let valid = vr.isValid;
        let subject = "";
        if (valid) {
            subject = vr.getSubject().trim();
        }
        vr.close();
        if (!valid || subject.length() == 0) {
            return empty;
        }
        return UserRepository.findById(subject);
    }
}
