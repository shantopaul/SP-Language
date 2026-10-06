import { NextResponse } from "next/server";
import { getPackageByName } from "@/data/packages-db";

export async function GET(
  request: Request,
  { params }: { params: { name: string } }
) {
  const pkg = getPackageByName(params.name);

  if (!pkg) {
    return NextResponse.json(
      { error: `Package '${params.name}' not found` },
      { status: 404 }
    );
  }

  return NextResponse.json({
    package: pkg,
  });
}
