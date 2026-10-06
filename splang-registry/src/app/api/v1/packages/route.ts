import { NextResponse } from "next/server";
import { PACKAGES } from "@/data/packages-db";

export async function GET(request: Request) {
  const { searchParams } = new URL(request.url);
  const category = searchParams.get("category");
  const sort = searchParams.get("sort") || "downloads";

  let list = [...PACKAGES];

  if (category) {
    list = list.filter((p) => p.category.toLowerCase() === category.toLowerCase());
  }

  if (sort === "downloads") {
    list.sort((a, b) => b.totalDownloads - a.totalDownloads);
  } else if (sort === "name") {
    list.sort((a, b) => a.name.localeCompare(b.name));
  } else if (sort === "newest") {
    list.sort((a, b) => b.publishedAt.localeCompare(a.publishedAt));
  }

  return NextResponse.json({
    total: list.length,
    packages: list,
  });
}
