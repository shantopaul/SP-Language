import React from "react";
import Link from "next/link";
import { Sparkles, Calendar, User, ArrowRight, BookOpen } from "lucide-react";
import { Badge } from "@/components/ui/Badge";

export const metadata = {
  title: "Blog & Announcements",
  description: "Official engineering blog, release announcements, and updates from the SP Programming Language team.",
};

const POSTS = [
  {
    slug: "v3.1.0-release",
    title: "Announcing SP 3.1.0: Enterprise Standard Library, Production Web Framework & Universal Toolchain",
    excerpt:
      "SP 3.1.0 GA arrives with 23 standard library modules including std.web delivering 185,000 req/s, asynchronous coroutines, native PostgreSQL TLS, automatic background upgrades via spc upgrade, and multi-drive Windows/Linux installation suites.",
    date: "October 10, 2026",
    author: "Shanto Paul",
    authorRole: "Language Creator & Lead Architect",
    category: "v3.1.0 GA",
    badgeVariant: "primary" as const,
    readTime: "7 min read",
  },
  {
    slug: "v1.0.0-release",
    title: "Announcing SP 1.0.0: The Production-Ready Systems & Application Language",
    excerpt:
      "Today marks a defining milestone in the evolution of SP. We are thrilled to announce the general availability of SP 1.0.0, featuring native LLVM codegen, package manager, developer tools, and comprehensive benchmarks.",
    date: "October 6, 2026",
    author: "Shanto Paul",
    authorRole: "Language Creator & Lead Architect",
    category: "v1.0.0 GA",
    badgeVariant: "secondary" as const,
    readTime: "6 min read",
  },
];

export default function BlogIndexPage() {
  return (
    <div className="py-16 max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 relative">
      {/* Background ambient glow */}
      <div className="absolute top-10 left-1/2 -translate-x-1/2 w-[600px] h-[300px] bg-gradient-to-b from-[#0057FF]/15 to-transparent rounded-full blur-[100px] pointer-events-none -z-10" />

      {/* Header */}
      <div className="text-center max-w-2xl mx-auto mb-16">
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-[#0057FF]/15 border border-[#00D1FF]/30 text-[#00D1FF] text-xs font-semibold mb-3 shadow-[0_0_15px_rgba(0,209,255,0.15)]">
          <BookOpen className="w-4 h-4 text-[#00D1FF]" />
          <span>Engineering Blog</span>
        </div>
        <h1 className="text-4xl sm:text-5xl font-black text-white tracking-tight">
          News & <span className="text-transparent bg-clip-text bg-gradient-to-r from-white via-slate-100 to-[#00D1FF]">Announcements</span>
        </h1>
        <p className="mt-3 text-base text-slate-300">
          Official release notes, architecture deep dives, and language design essays from the core team.
        </p>
      </div>

      {/* Posts List */}
      <div className="space-y-8">
        {POSTS.map((post) => (
          <article
            key={post.slug}
            className="rounded-2xl border border-[#0A1F44] bg-[#081735]/60 backdrop-blur-md p-6 sm:p-8 hover:border-[#0057FF]/50 transition-all shadow-xl group relative overflow-hidden"
          >
            <div className="flex flex-wrap items-center gap-3 mb-4 text-xs">
              <Badge variant={post.badgeVariant}>{post.category}</Badge>
              <span className="flex items-center gap-1.5 text-slate-400">
                <Calendar className="w-3.5 h-3.5 text-[#00D1FF]" />
                {post.date}
              </span>
              <span className="text-[#0A1F44]">•</span>
              <span className="flex items-center gap-1.5 text-slate-400">
                <User className="w-3.5 h-3.5 text-[#00D1FF]" />
                {post.author} ({post.authorRole})
              </span>
              <span className="text-[#0A1F44]">•</span>
              <span className="text-slate-500 font-mono">{post.readTime}</span>
            </div>

            <Link href={`/blog/${post.slug}`}>
              <h2 className="text-2xl sm:text-3xl font-bold text-white group-hover:text-[#00D1FF] transition-colors mb-3 leading-snug">
                {post.title}
              </h2>
            </Link>

            <p className="text-slate-300 text-sm sm:text-base leading-relaxed mb-6">
              {post.excerpt}
            </p>

            <Link
              href={`/blog/${post.slug}`}
              className="inline-flex items-center gap-1.5 text-sm font-semibold text-[#00D1FF] group-hover:text-cyan-300 transition-colors"
            >
              <span>Read Full Announcement</span>
              <ArrowRight className="w-4 h-4 group-hover:translate-x-1 transition-transform" />
            </Link>
          </article>
        ))}
      </div>
    </div>
  );
}
