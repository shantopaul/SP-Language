import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.security & std.crypto Module Reference",
  description: "RFC 9106 Argon2id password hashing, JWT HS256 tokens, and constant-time cryptography.",
};

export default function StdSecurityPage() {
  const exampleCode = `import std.security;
import std.crypto;

fn main() {
    let hash = Password.hash("super_secret");
    let verified = Password.verify("super_secret", hash);
    println("Password verified: " + (verified ? "YES" : "NO"));

    let token = Jwt.sign("{\\"sub\\":\\"123\\"}", "secret_key");
    println("JWT Token: " + token);
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.security & std.crypto" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.security &amp; std.crypto Reference</h1>
      <p className="text-sm text-slate-300">
        Provides enterprise cryptographic primitives including memory-hard Argon2id password hashing, JWT HMAC-SHA256 token creation and validation, and timing-safe byte equality.
      </p>

      <CodeBlock code={exampleCode} filename="security_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="Password.hash"
        signature="static fn hash(string password) -> string"
        description="Generates an RFC 9106 compliant Argon2id hash containing salt, parameters, and digest."
        parameters={[{ name: "password", type: "string", description: "Plaintext password string." }]}
        returns="Encoded Argon2id hash string ($argon2id$...)"
      />

      <ApiCard
        name="Password.verify"
        signature="static fn verify(string password, string hash) -> bool"
        description="Verifies plaintext password against an encoded Argon2id hash in constant time."
        parameters={[
          { name: "password", type: "string", description: "Plaintext password to test." },
          { name: "hash", type: "string", description: "Stored Argon2id hash string." },
        ]}
        returns="bool match status"
      />

      <ApiCard
        name="Jwt.sign"
        signature="static fn sign(string payloadJson, string secret) -> string"
        description="Signs a JSON payload string with HMAC-SHA256, returning compact Header.Payload.Signature."
        parameters={[
          { name: "payloadJson", type: "string", description: "Valid JSON claims string." },
          { name: "secret", type: "string", description: "Symmetric secret key string." },
        ]}
        returns="Compact JWT token string"
      />

      <ApiCard
        name="Jwt.verify"
        signature="static fn verify(string token, string secret) -> string"
        description="Validates signature and exp timestamp, returning decoded payload on success or empty string on failure."
        parameters={[
          { name: "token", type: "string", description: "Encoded JWT token string." },
          { name: "secret", type: "string", description: "Symmetric secret key string." },
        ]}
        returns="Decoded payload JSON string, or empty string on error"
      />

      <PrevNextNav
        prev={{ title: "std.redis Reference", href: "/stdlib/redis" }}
        next={{ title: "std.net.ws Reference", href: "/stdlib/ws" }}
      />
    </div>
  );
}
