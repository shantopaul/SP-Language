import { NextResponse } from "next/server";
import { searchPackages } from "@/data/packages-db";

export async function GET(request: Request) {
  const { searchParams } = new URL(request.url);
  const q = searchParams.get("q") || "";
  const category = searchParams.get("category") || undefined;

  const matches = searchPackages(q, category);

  return NextResponse.json({
    query: q,
    count: matches.length,
    results: matches.map((p) => ({
      name: p.name,
      version: p.version,
      description: p.description,
      downloads: p.totalDownloads,
      author: p.author,
      official: p.official,
    })),
  });
}
