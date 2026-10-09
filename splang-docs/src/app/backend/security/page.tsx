import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { ShieldCheck, Lock, Key, ShieldAlert } from "lucide-react";

export const metadata = {
  title: "Security, JWT & Argon2id (std.security & std.crypto)",
  description: "Enterprise authentication, password hashing, and token cryptography in SP 3.0.",
};

export default function BackendSecurityPage() {
  const securityAuthCode = `import std.io;
import std.web;
import std.security;
import std.crypto;

let app = new App();
let JWT_SECRET = "super-secret-key-32-chars-long!";

// 1. User Registration with Argon2id Password Hashing
app.post("/api/auth/register", (ptr req) => {
    let ctx = Context.create(req);
    let plainPassword = "user-secret-password";

    // RFC 9106 compliant memory-hard password hashing
    let passwordHash = Password.hash(plainPassword);

    return ctx.json("{\\"status\\":\\"registered\\",\\"hash\\":\\"" + passwordHash + "\\"}", 201);
});

// 2. User Login & JWT Token Minting
app.post("/api/auth/login", (ptr req) => {
    let ctx = Context.create(req);

    // Verify password against stored Argon2id hash
    let isValid = Password.verify("user-secret-password", storedHash);
    if (!isValid) {
        return ctx.json("{\\"error\\":\\"Invalid credentials\\"}", 401);
    }

    // Sign JWT HS256 Token (1 hour expiration)
    let payload = "{\\"sub\\":\\"108\\",\\"role\\":\\"admin\\",\\"exp\\":1792000000}";
    let token = Jwt.sign(payload, JWT_SECRET);

    return ctx.json("{\\"token\\":\\"" + token + "\\"}");
});

// 3. Protected Endpoint with JWT Verification
app.get("/api/admin/dashboard", (ptr req) => {
    let ctx = Context.create(req);
    let authHeader = ctx.header("Authorization");

    if (!authHeader.startsWith("Bearer ")) {
        return ctx.json("{\\"error\\":\\"Missing Bearer token\\"}", 401);
    }

    let token = authHeader.substring(7);
    let verifiedPayload = Jwt.verify(token, JWT_SECRET);

    if (verifiedPayload == "") {
        return ctx.json("{\\"error\\":\\"Invalid or expired token\\"}", 403);
    }

    return ctx.json("{\\"welcome\\":\\"admin\\",\\"claims\\":" + verifiedPayload + "}");
});

app.listen(8080);`;

  const timingSafeCode = `// Constant-Time String Comparison
// Prevents side-channel timing attacks when validating API tokens or signatures
let userToken = ctx.header("X-Api-Key");
let validToken = "prod_api_key_8849204128491";

let isMatch = Crypto.timingSafeEqual(userToken, validToken);
if (!isMatch) {
    return ctx.json("{\\"error\\":\\"Unauthorized\\"}", 401);
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 3.0 Backend Platform", href: "/backend/overview" },
          { title: "Security, JWT & Argon2id" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <ShieldCheck className="w-3.5 h-3.5" />
          <span>Cryptography &bull; std.security &amp; std.crypto</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Security, JWT &amp; Argon2id Cryptography
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          SP provides native security primitives designed to withstand modern cryptographic attacks. Hash passwords with memory-hard Argon2id, issue signed JSON Web Tokens (JWT HS256), and prevent side-channel timing attacks using constant-time comparisons.
        </p>
      </div>

      <Callout type="tip" title="RFC 9106 Argon2id Standard">
        Argon2id combines resistance against GPU side-channel attacks and cache-timing attacks, providing industry-leading protection against brute-force password cracking.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Authentication Pipeline Example</h2>
        <p className="text-xs text-slate-400 mb-3">
          Complete authentication flow including Argon2id hashing, verification, and JWT minting:
        </p>
        <CodeBlock code={securityAuthCode} filename="src/auth_service.sp" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Constant-Time Timing Attack Defense</h2>
        <p className="text-xs text-slate-400 mb-3">
          Standard string equality (<code>==</code>) short-circuits on the first mismatching byte, leaking secret information through microsecond timing discrepancies. SP provides <code>Crypto.timingSafeEqual</code>:
        </p>
        <CodeBlock code={timingSafeCode} filename="src/timing_defense.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Security API Reference</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="border-b border-slate-800 text-slate-400">
              <tr>
                <th className="pb-2 font-sans font-semibold">Method</th>
                <th className="pb-2 font-sans font-semibold">Description</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 text-slate-300">
              <tr>
                <td className="py-2 text-cyan-400">Password.hash(plainText)</td>
                <td className="py-2">Computes RFC 9106 Argon2id salt + hash with tuned time/memory costs</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Password.verify(plainText, hash)</td>
                <td className="py-2">Constant-time verification of password against encoded hash string</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Jwt.sign(payloadJson, secret)</td>
                <td className="py-2">Generates compact <code>Header.Payload.Signature</code> token with HMAC-SHA256</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Jwt.verify(token, secret)</td>
                <td className="py-2">Verifies signature &amp; expiration, returning decoded payload or empty string</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Crypto.timingSafeEqual(a, b)</td>
                <td className="py-2">Constant-time byte comparison resistant to timing side-channels</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Crypto.sha256(data)</td>
                <td className="py-2">Computes cryptographic SHA-256 digest hex string</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Database & Pooling", href: "/backend/database" }}
        next={{ title: "WebSockets & Real-Time", href: "/backend/websockets" }}
      />
    </div>
  );
}
