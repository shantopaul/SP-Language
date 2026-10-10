import React from "react";
import { Terminal, Shield } from "lucide-react";

export interface Parameter {
  name: string;
  type: string;
  description: string;
  optional?: boolean;
}

export interface ApiCardProps {
  signature: string;
  name: string;
  description: string;
  parameters?: Parameter[];
  returns?: string;
  example?: string;
}

export const ApiCard: React.FC<ApiCardProps> = ({
  signature,
  name,
  description,
  parameters = [],
  returns,
  example,
}) => {
  return (
    <div className="rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 p-5 my-6 shadow-sm dark:shadow-xl hover:border-blue-300 dark:hover:border-[#0057FF]/50 transition-all">
      <div className="flex items-center justify-between gap-3 mb-2 flex-wrap">
        <h4 className="text-base font-bold text-slate-900 dark:text-white font-mono">{name}</h4>
        <span className="text-[11px] font-mono px-2 py-0.5 rounded-full bg-blue-50 dark:bg-[#0057FF]/20 text-[#0057FF] dark:text-[#00D1FF] border border-blue-200 dark:border-[#00D1FF]/30 font-semibold">
          Native Builtin
        </span>
      </div>

      <div className="bg-[#0B1528] dark:bg-[#020D21] p-3 rounded-lg border border-slate-300 dark:border-[#0A1F44] font-mono text-xs text-[#60A5FA] dark:text-[#00D1FF] mb-3 overflow-x-auto whitespace-pre font-semibold">
        {signature}
      </div>

      <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-300 mb-4 leading-relaxed">
        {description}
      </p>

      {parameters.length > 0 && (
        <div className="mb-4">
          <h5 className="text-xs uppercase tracking-wider font-semibold text-slate-500 dark:text-slate-400 mb-2">
            Parameters
          </h5>
          <div className="divide-y divide-slate-200 dark:divide-[#0A1F44]/60 border border-slate-200 dark:border-[#0A1F44] rounded-lg overflow-hidden bg-slate-50/50 dark:bg-[#020D21]/60">
            {parameters.map((param) => (
              <div key={param.name} className="p-2.5 text-xs flex flex-col sm:flex-row sm:items-center justify-between gap-1">
                <div className="flex items-center gap-2">
                  <span className="font-mono text-[#0057FF] dark:text-[#00D1FF] font-semibold">{param.name}</span>
                  <span className="font-mono text-slate-500 dark:text-slate-400 text-[11px]">({param.type})</span>
                  {param.optional && (
                    <span className="text-[10px] text-amber-700 dark:text-amber-300 px-1.5 py-0.5 rounded-full bg-amber-50 dark:bg-amber-950/60 border border-amber-200 dark:border-amber-800 font-semibold">
                      optional
                    </span>
                  )}
                </div>
                <span className="text-slate-600 dark:text-slate-400 text-xs">{param.description}</span>
              </div>
            ))}
          </div>
        </div>
      )}

      {returns && (
        <div className="text-xs flex items-center gap-2 text-slate-500 dark:text-slate-400">
          <span className="font-semibold text-slate-700 dark:text-slate-300 uppercase tracking-wider text-[11px]">
            Returns:
          </span>
          <code className="font-mono text-[#0057FF] dark:text-[#00D1FF] bg-slate-100 dark:bg-[#020D21] px-2 py-0.5 rounded border border-slate-200 dark:border-[#0A1F44] font-semibold">
            {returns}
          </code>
        </div>
      )}
    </div>
  );
};
