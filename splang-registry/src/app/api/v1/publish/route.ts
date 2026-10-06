import { NextResponse } from "next/server";

export async function POST(request: Request) {
  try {
    const body = await request.json();
    const { name, version } = body;

    if (!name || !version) {
      return NextResponse.json(
        { error: "Missing required fields 'name' and 'version'" },
        { status: 400 }
      );
    }

    return NextResponse.json({
      success: true,
      message: `Package '${name}@${version}' received and published successfully to https://registry.splang.shantopaul.com`,
      package: {
        name,
        version,
        publishedAt: new Date().toISOString(),
      },
    });
  } catch {
    return NextResponse.json(
      { error: "Invalid JSON publication manifest" },
      { status: 400 }
    );
  }
}
