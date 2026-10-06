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
    slug: "v1.0.0-release",
    title: "Announcing SP 1.0.0: The Production-Ready Systems & Application Language",
    excerpt:
      "Today marks a defining milestone in the evolution of SP. We are thrilled to announce the general availability of SP 1.0.0, featuring native LLVM codegen, package manager, developer tools, and comprehensive benchmarks.",
    date: "October 6, 2026",
    author: "Shanto Paul",
    authorRole: "Language Creator & Lead Architect",
    category: "Release",
    readTime: "6 min read",
  },
];

export default function BlogIndexPage() {
  return (
    <div className="py-16 max-w-5xl mx-auto px-4 sm:px-6 lg:px-8">
      {/* Header */}
      <div className="text-center max-w-2xl mx-auto mb-16">
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-3">
          <BookOpen className="w-4 h-4" />
          <span>Engineering Blog</span>
        </div>
        <h1 className="text-4xl sm:text-5xl font-extrabold text-white tracking-tight">
          News & Updates
        </h1>
        <p className="mt-3 text-base text-slate-400">
          Official release notes, architecture deep dives, and language design essays from the core team.
        </p>
      </div>

      {/* Posts List */}
      <div className="space-y-8">
        {POSTS.map((post) => (
          <article
            key={post.slug}
            className="rounded-2xl border border-slate-800 bg-slate-900/40 p-6 sm:p-8 hover:border-slate-700 transition-all shadow-xl group"
          >
            <div className="flex flex-wrap items-center gap-3 mb-4 text-xs">
              <Badge variant="success">{post.category}</Badge>
              <span className="flex items-center gap-1.5 text-slate-400">
                <Calendar className="w-3.5 h-3.5" />
                {post.date}
              </span>
              <span className="text-slate-600">•</span>
              <span className="flex items-center gap-1.5 text-slate-400">
                <User className="w-3.5 h-3.5" />
                {post.author} ({post.authorRole})
              </span>
              <span className="text-slate-600">•</span>
              <span className="text-slate-500">{post.readTime}</span>
            </div>

            <Link href={`/blog/${post.slug}`}>
              <h2 className="text-2xl sm:text-3xl font-bold text-white group-hover:text-cyan-400 transition-colors mb-3 leading-snug">
                {post.title}
              </h2>
            </Link>

            <p className="text-slate-300 text-sm sm:text-base leading-relaxed mb-6">
              {post.excerpt}
            </p>

            <Link
              href={`/blog/${post.slug}`}
              className="inline-flex items-center gap-1.5 text-sm font-semibold text-cyan-400 hover:text-cyan-300 transition-colors"
            >
              <span>Read Full Article</span>
              <ArrowRight className="w-4 h-4 group-hover:translate-x-1 transition-transform" />
            </Link>
          </article>
        ))}
      </div>
    </div>
  );
}
