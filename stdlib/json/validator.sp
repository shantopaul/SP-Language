module std.json.validator;

// ============================================================================
// Standard Library JSON Schema Validator & RFC 7807 Generator (SP 2.4)
// ============================================================================

import std.json;

public class ValidationError {
    public string field;
    public string message;
    public string code;

    public ValidationError(string field, string message, string code) {
        this.field = field;
        this.message = message;
        this.code = code;
    }

    public fn toJson() -> string {
        return "{\"field\":\"" + this.field + "\",\"message\":\"" + this.message + "\",\"code\":\"" + this.code + "\"}";
    }
}

public class ValidationResult {
    public bool isValid;
    public string errorsJson;
    private int errorCount;

    public ValidationResult() {
        this.isValid = true;
        this.errorsJson = "[]";
        this.errorCount = 0;
    }

    public fn addError(string field, string message, string code) {
        this.isValid = false;
        let errJson = "{\"field\":\"" + field + "\",\"message\":\"" + message + "\",\"code\":\"" + code + "\"}";
        if (this.errorCount == 0) {
            this.errorsJson = "[" + errJson + "]";
        } else {
            let current = this.errorsJson;
            let sub = current.substring(0, current.length() - 1);
            this.errorsJson = sub + "," + errJson + "]";
        }
        this.errorCount = this.errorCount + 1;
    }

    public fn getErrorCount() -> int {
        return this.errorCount;
    }

    public fn toProblemDetails(string title, string detail, string instance) -> string {
        return Json.problemDetails(title, 422, detail, instance, this.errorsJson);
    }
}

public class Validator {
    private ValidationResult result;

    public Validator() {
        this.result = new ValidationResult();
    }

    public fn required(string field, string value, string message) -> Validator {
        if (value == "") {
            this.result.addError(field, message != "" ? message : "${field} is required", "REQUIRED");
        }
        return this;
    }

    public fn minLength(string field, string value, int minLen, string message) -> Validator {
        if (value.length() < minLen) {
            this.result.addError(field, message != "" ? message : "${field} must be at least ${minLen} characters", "MIN_LENGTH");
        }
        return this;
    }

    public fn maxLength(string field, string value, int maxLen, string message) -> Validator {
        if (value.length() > maxLen) {
            this.result.addError(field, message != "" ? message : "${field} must be at most ${maxLen} characters", "MAX_LENGTH");
        }
        return this;
    }

    public fn range(string field, int value, int minVal, int maxVal, string message) -> Validator {
        if (value < minVal || value > maxVal) {
            this.result.addError(field, message != "" ? message : "${field} must be between ${minVal} and ${maxVal}", "RANGE");
        }
        return this;
    }

    public fn email(string field, string value, string message) -> Validator {
        if (value != "") {
            if (!value.contains("@") || !value.contains(".")) {
                this.result.addError(field, message != "" ? message : "${field} must be a valid email address", "INVALID_EMAIL");
            }
        }
        return this;
    }

    public fn custom(string field, bool isValid, string message, string code) -> Validator {
        if (!isValid) {
            this.result.addError(field, message, code != "" ? code : "CUSTOM_VALIDATION");
        }
        return this;
    }

    public fn validate() -> ValidationResult {
        return this.result;
    }
}
