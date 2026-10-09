module examples.security_auth;

import std.io;
import std.crypto;
import std.crypto.password;
import std.security.jwt;
import std.security.session;
import std.security.middleware;

fn main() {
    println("==================================================================");
    println("   SP Language - Phase 24: Security, Authentication & Cryptography");
    println("==================================================================");
    println("");

    // ------------------------------------------------------------------------
    // 1. Password Hashing (Argon2id & Bcrypt)
    // ------------------------------------------------------------------------
    println("[1] Password Hashing (Argon2id & Bcrypt):");
    var rawPassword = "CorrectHorseBatteryStaple#2026!";

    // Argon2id (RFC 9106)
    var argonHash = Password.argon2id(rawPassword, 1024, 2, 1);
    println("    * Argon2id PHC Hash: " + argonHash);
    var argonValid = Password.verify(rawPassword, argonHash);
    var argonWrong = Password.verify("WrongPassword", argonHash);
    println("    * Argon2id Verify (correct): " + (argonValid ? "true" : "false"));
    println("    * Argon2id Verify (wrong):   " + (argonWrong ? "true" : "false"));

    // Bcrypt (Modular Crypt Format)
    var bcryptHash = Password.bcrypt(rawPassword, 4);
    println("    * Bcrypt MCF Hash:   " + bcryptHash);
    var bcryptValid = Password.verify(rawPassword, bcryptHash);
    println("    * Bcrypt Verify (correct):   " + (bcryptValid ? "true" : "false"));
    println("");

    // ------------------------------------------------------------------------
    // 2. JSON Web Tokens (RFC 7519, HS256)
    // ------------------------------------------------------------------------
    println("[2] JSON Web Tokens (RFC 7519):");
    var jwtSecret = "super-secret-key-for-sp-jwt-engine-32b!";
    
    var claims = new JwtClaims();
    claims.init();
    claims.setSubject("usr_882910");
    claims.setIssuer("https://auth.splang.io");
    claims.setAudience("https://api.splang.io");
    claims.setExpiresIn(3600); // 1 hour
    claims.setPayload("{\"sub\":\"usr_882910\",\"role\":\"admin\",\"dept\":\"security\"}");

    var token = Jwt.signClaims(claims, jwtSecret);
    println("    * Generated JWT: " + token);

    // Verify token
    var verifyRes = Jwt.verifyWithExpectations(token, jwtSecret, "https://auth.splang.io", "https://api.splang.io");
    println("    * Token Valid?       " + (verifyRes.isValid ? "true" : "false"));
    println("    * Extracted Subject: " + verifyRes.getSubject());
    println("    * Extracted Issuer:  " + verifyRes.getIssuer());
    println("    * Extracted Role:    " + verifyRes.getClaim("role"));

    // Tamper test
    var tamperedToken = token + "x";
    var tamperedRes = Jwt.verify(tamperedToken, jwtSecret);
    println("    * Tampered Token Valid? " + (tamperedRes.isValid ? "true" : "false") + " (" + tamperedRes.error + ")");
    verifyRes.close();
    tamperedRes.close();
    println("");

    // ------------------------------------------------------------------------
    // 3. Signed Cookies & Tamper Resistance
    // ------------------------------------------------------------------------
    println("[3] Signed Cookies & Tamper Resistance:");
    var cookieSecret = "session-cookie-hmac-secret-key!";
    var sessionVal = "session_live_992182";

    var signedCookieVal = Cookie.sign(sessionVal, cookieSecret);
    println("    * Signed Value: " + signedCookieVal);

    var unsignedSuccess = Cookie.unsign(signedCookieVal, cookieSecret);
    println("    * Unsigned (valid key):   " + unsignedSuccess);

    var unsignedTampered = Cookie.unsign(signedCookieVal + "corrupt", cookieSecret);
    var unsignedWrongKey = Cookie.unsign(signedCookieVal, "wrong-key");
    println("    * Unsigned (tampered):    '" + unsignedTampered + "'");
    println("    * Unsigned (wrong secret): '" + unsignedWrongKey + "'");

    var cookie = Cookie.create("sessionId", signedCookieVal);
    cookie.setPath("/");
    cookie.setMaxAge(3600);
    cookie.setHttpOnly(true);
    cookie.setSecure(true);
    cookie.setSameSite("Strict");
    println("    * Set-Cookie Header: " + cookie.serialize());
    println("");

    // ------------------------------------------------------------------------
    // 4. In-Memory Session Store & Fixation Protection
    // ------------------------------------------------------------------------
    println("[4] Session Store & Session Fixation Protection:");
    var store = SessionStore.create(1800);
    var sid1 = store.createSession("{\"userId\":\"usr_882910\",\"authenticated\":true}");
    println("    * Created Session ID 1: " + sid1);
    println("    * Session 1 Data:       " + store.get(sid1));

    // Regenerate session ID (protect against session fixation attacks)
    var sid2 = store.regenerate(sid1);
    println("    * Regenerated Session 2: " + sid2);
    println("    * Old Session 1 Active? " + (store.get(sid1) != "" ? "true" : "false"));
    println("    * New Session 2 Active? " + (store.get(sid2) != "" ? "true" : "false"));
    println("    * Session 2 Data:        " + store.get(sid2));
    store.destroy(sid2);
    store.close();
    println("");

    // ------------------------------------------------------------------------
    // 5. Constant-Time String Comparison
    // ------------------------------------------------------------------------
    println("[5] Constant-Time Equality (Crypto.timingSafeEqual):");
    var keyA = "8f3b2c1a0d4e5f6a7b8c9d0e1f2a3b4c";
    var keyB = "8f3b2c1a0d4e5f6a7b8c9d0e1f2a3b4c";
    var keyC = "8f3b2c1a0d4e5f6a7b8c9d0e1f2a3b4d";
    println("    * Match (keyA, keyB): " + (Crypto.timingSafeEqual(keyA, keyB) ? "true" : "false"));
    println("    * Diff  (keyA, keyC): " + (Crypto.timingSafeEqual(keyA, keyC) ? "true" : "false"));
    println("");

    println("==================================================================");
    println("   Security, Auth & Cryptography (Phase 24) Verified Successfully!");
    println("==================================================================");
}
