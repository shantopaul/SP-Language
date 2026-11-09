import { NextResponse } from "next/server";
import { getPackageByName, REGISTRY_PREVIEW } from "@/data/packages-db";

export async function GET(
  request: Request,
  { params }: { params: { name: string; version: string } }
) {
  const pkg = getPackageByName(params.name);

  if (!pkg) {
    return NextResponse.json(
      { error: `Package '${params.name}' not found` },
      { status: 404 }
    );
  }

  const ver = pkg.versions.find((v) => v.version === params.version);

  if (!ver) {
    return NextResponse.json(
      { error: `Version '${params.version}' of package '${params.name}' not found` },
      { status: 404 }
    );
  }

  // Preview/mock only: the checksum below comes from a hard-coded preview
  // catalogue. No file was downloaded or hashed, so this is NOT a real
  // integrity verification. Do not trust `verified` here.
  return NextResponse.json({
    name: pkg.name,
    version: ver.version,
    checksum: ver.checksum,
    algorithm: "SHA-256",
    verified: false,
    preview: REGISTRY_PREVIEW.preview,
    mock: REGISTRY_PREVIEW.mock,
    message:
      "SP Registry is a preview/mock: this checksum is from a hard-coded preview catalogue. " +
      "No file was downloaded or hashed, so nothing has actually been verified.",
  });
}
