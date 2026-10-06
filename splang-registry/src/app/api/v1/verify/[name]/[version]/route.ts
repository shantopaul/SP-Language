import { NextResponse } from "next/server";
import { getPackageByName } from "@/data/packages-db";

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

  return NextResponse.json({
    name: pkg.name,
    version: ver.version,
    checksum: ver.checksum,
    algorithm: "SHA-256",
    verified: true,
  });
}
