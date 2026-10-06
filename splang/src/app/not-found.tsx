import React from "react";
import Link from "next/link";
import { ArrowLeft, Compass } from "lucide-react";
import { Button } from "@/components/ui/Button";

export default function NotFound() {
  return (
    <div className="min-h-[70vh] flex items-center justify-center px-4">
      <div className="text-center max-w-md">
        <div className="w-16 h-16 rounded-2xl bg-cyan-950/60 border border-cyan-800/80 flex items-center justify-center mx-auto mb-6 text-cyan-400">
          <Compass className="w-8 h-8" />
        </div>
        <span className="font-mono text-cyan-400 text-sm font-semibold">Error 404</span>
        <h1 className="text-3xl font-extrabold text-white mt-1 mb-3">Page Not Found</h1>
        <p className="text-sm text-slate-400 mb-8 leading-relaxed">
          The requested page could not be found. Please check the URL or navigate back to the main portal or documentation.
        </p>
        <div className="flex items-center justify-center gap-3">
          <Link href="/">
            <Button variant="primary">
              <ArrowLeft className="w-4 h-4 mr-1.5" />
              <span>Back to Home</span>
            </Button>
          </Link>
          <a
            href="https://docs.splang.shantopaul.com"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button variant="secondary">Go to Docs</Button>
          </a>
        </div>
      </div>
    </div>
  );
}
