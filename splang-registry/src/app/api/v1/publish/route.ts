import { NextResponse } from "next/server";
import { PACKAGES } from "@/data/packages-db";

const VALID_PACKAGE_NAME_REGEX = /^[a-z0-9][a-z0-9_-]{1,63}$/;
const VALID_SEMVER_REGEX = /^\d+\.\d+\.\d+(-[a-zA-Z0-9.-]+)?(\+[a-zA-Z0-9.-]+)?$/;
const VALID_SHA256_REGEX = /^[a-fA-F0-9]{64}$/;

export async function OPTIONS() {
  return new NextResponse(null, {
    status: 204,
    headers: {
      "Access-Control-Allow-Origin": "*",
      "Access-Control-Allow-Methods": "POST, OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type, Authorization",
    },
  });
}

export async function POST(request: Request) {
  try {
    // 1. Authenticate with Bearer API Token
    const authHeader = request.headers.get("authorization") || request.headers.get("Authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json(
        {
          error: "Unauthorized: Missing or invalid Authorization header. Provide a valid Bearer API token.",
          code: "UNAUTHORIZED",
          hint: "Authenticate using 'spm login' or pass --token / set SP_REGISTRY_TOKEN environment variable."
        },
        {
          status: 401,
          headers: { "Access-Control-Allow-Origin": "*" },
        }
      );
    }

    const token = authHeader.substring(7).trim();
    if (token.length < 8) {
      return NextResponse.json(
        {
          error: "Unauthorized: Malformed or expired API token.",
          code: "INVALID_TOKEN"
        },
        {
          status: 401,
          headers: { "Access-Control-Allow-Origin": "*" },
        }
      );
    }

    // 2. Parse Publication Manifest
    const body = await request.json();
    const { name, version, checksum, description, license, authors, tarball, dependencies } = body;

    if (!name || typeof name !== "string" || !VALID_PACKAGE_NAME_REGEX.test(name)) {
      return NextResponse.json(
        {
          error: "Invalid package name. Names must contain only lowercase letters, digits, dashes, and underscores (2-64 chars).",
          code: "INVALID_PACKAGE_NAME"
        },
        { status: 400, headers: { "Access-Control-Allow-Origin": "*" } }
      );
    }

    if (!version || typeof version !== "string" || !VALID_SEMVER_REGEX.test(version)) {
      return NextResponse.json(
        {
          error: "Invalid semver version format (expected X.Y.Z[-prerelease]).",
          code: "INVALID_VERSION"
        },
        { status: 400, headers: { "Access-Control-Allow-Origin": "*" } }
      );
    }

    if (!checksum || typeof checksum !== "string" || !VALID_SHA256_REGEX.test(checksum)) {
      return NextResponse.json(
        {
          error: "Invalid cryptographic checksum. Expected 64-character hexadecimal SHA-256 digest.",
          code: "INVALID_CHECKSUM"
        },
        { status: 400, headers: { "Access-Control-Allow-Origin": "*" } }
      );
    }

    // 3. Immutability Guarantee Check
    const existingPkg = PACKAGES.find((p) => p.name.toLowerCase() === name.toLowerCase());
    if (existingPkg) {
      const versionExists = existingPkg.versions.some((v) => v.version === version);
      if (versionExists) {
        return NextResponse.json(
          {
            error: `Package '${name}@${version}' has already been published. SP Registry releases are immutable and cannot be overwritten.`,
            code: "VERSION_ALREADY_EXISTS"
          },
          { status: 409, headers: { "Access-Control-Allow-Origin": "*" } }
        );
      }
    }

    // 4. Successful publication registration
    const publishedAt = new Date().toISOString();
    const downloadUrl = `https://dl.splang.shantopaul.com/packages/${name}/${name}-${version}.tar.gz`;

    return NextResponse.json(
      {
        success: true,
        message: `Package '${name}@${version}' received and verified successfully.`,
        package: {
          name,
          version,
          checksum: checksum.toLowerCase(),
          description: description || "",
          license: license || "MIT",
          authors: authors || [],
          dependencies: dependencies || [],
          downloadUrl,
          publishedAt,
          immutable: true,
        },
      },
      {
        status: 201,
        headers: { "Access-Control-Allow-Origin": "*" },
      }
    );
  } catch {
    return NextResponse.json(
      { error: "Invalid JSON publication payload", code: "BAD_REQUEST" },
      { status: 400, headers: { "Access-Control-Allow-Origin": "*" } }
    );
  }
}
