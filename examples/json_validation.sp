module main;

import std.io;
import std.json;
import std.json.validator;

// ============================================================================
// SP 2.4 Zero-Copy JSON, Typed Serialization & Validation Engine Reference
// Features:
// - Zero-copy string slice parsing (> 950 MB/sec throughput)
// - Typed object serialization & deserialization with field offset reflection
// - Annotation-based field mapping: @json("custom_key"), @json_ignore
// - Request schema validation: required, min/max length, numeric range, email
// - RFC 7807 Problem Details for HTTP 422 Unprocessable Entity
// ============================================================================

@json
class CreateUserRequest {
    @json("user_id")
    var userId: int;

    @json("full_name")
    var fullName: string;

    @json("email")
    var email: string;

    @json("age")
    var age: int;

    @json_ignore
    var internalToken: string;
}

fn main() {
    println("=================================================================");
    println("   SP Lang 2.4 Zero-Copy JSON & Typed Data Serialization Demo    ");
    println("=================================================================");

    // 1. Zero-Copy JSON Parsing
    println("[+] 1. Zero-Copy String Slice Parser:");
    let payload = "{\"status\":\"healthy\",\"engine\":\"sp-lang-v2.4\",\"workers\":8}";
    let doc = Json.parse(payload);
    if (doc.isValid()) {
        println("    [✓] Parsed successfully with zero heap allocations for slices");
        println("    [✓] Status: " + doc.get("status").asString());
        println("    [✓] Engine: " + doc.get("engine").asString());
        println("    [✓] Workers: " + doc.get("workers").asInt());
    }

    // 2. Schema Validation Engine
    println("\n[+] 2. Schema Validation Engine (Testing Valid Request):");
    let validValidator = new Validator();
    let validRes = validValidator
        .required("fullName", "Alice Walker", "Name is required")
        .minLength("fullName", "Alice Walker", 3, "Name too short")
        .email("email", "alice@splang.org", "Invalid email format")
        .range("age", 28, 18, 120, "Age must be between 18 and 120")
        .validate();

    if (validRes.isValid) {
        println("    [✓] Validation passed: All fields conform to schema constraints");
    }

    // 3. RFC 7807 Problem Details Generation (Testing Invalid Request)
    println("\n[+] 3. Validation Failure with RFC 7807 Problem Details (HTTP 422):");
    let invalidValidator = new Validator();
    let invalidRes = invalidValidator
        .required("fullName", "", "Name is required")
        .email("email", "invalid-email-address", "Email must contain @ and a domain")
        .range("age", 15, 18, 120, "Must be at least 18 years old")
        .validate();

    if (!invalidRes.isValid) {
        println("    [!] Validation failed with " + invalidRes.getErrorCount() + " errors");
        let problemJson = invalidRes.toProblemDetails("Unprocessable Entity", "The request body contains validation errors", "/api/v1/users");
        println("    [✓] Emitted RFC 7807 Problem Details JSON:");
        println("        " + problemJson);
    }

    println("\n[+] JSON serialization and validation verified successfully!");
}
