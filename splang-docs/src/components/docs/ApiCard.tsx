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
    <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5 my-6 shadow-md hover:border-slate-700 transition-colors">
      <div className="flex items-center justify-between gap-3 mb-2 flex-wrap">
        <h4 className="text-base font-bold text-white font-mono">{name}</h4>
        <span className="text-[11px] font-mono px-2 py-0.5 rounded bg-emerald-950/80 text-emerald-400 border border-emerald-800/80">
          Native Builtin
        </span>
      </div>

      <div className="bg-black/60 p-3 rounded-lg border border-slate-800 font-mono text-xs text-cyan-300 mb-3 overflow-x-auto whitespace-pre">
        {signature}
      </div>

      <p className="text-xs sm:text-sm text-slate-300 mb-4 leading-relaxed">
        {description}
      </p>

      {parameters.length > 0 && (
        <div className="mb-4">
          <h5 className="text-xs uppercase tracking-wider font-semibold text-slate-400 mb-2">
            Parameters
          </h5>
          <div className="divide-y divide-slate-800/60 border border-slate-800 rounded-lg overflow-hidden bg-slate-950/40">
            {parameters.map((param) => (
              <div key={param.name} className="p-2.5 text-xs flex flex-col sm:flex-row sm:items-center justify-between gap-1">
                <div className="flex items-center gap-2">
                  <span className="font-mono text-emerald-400 font-semibold">{param.name}</span>
                  <span className="font-mono text-slate-500 text-[11px]">({param.type})</span>
                  {param.optional && (
                    <span className="text-[10px] text-amber-400 px-1.5 py-0.2 rounded bg-amber-950 border border-amber-900">
                      optional
                    </span>
                  )}
                </div>
                <span className="text-slate-400 text-xs">{param.description}</span>
              </div>
            ))}
          </div>
        </div>
      )}

      {returns && (
        <div className="text-xs flex items-center gap-2 text-slate-400">
          <span className="font-semibold text-slate-300 uppercase tracking-wider text-[11px]">
            Returns:
          </span>
          <code className="font-mono text-cyan-400 bg-slate-950 px-2 py-0.5 rounded border border-slate-800">
            {returns}
          </code>
        </div>
      )}
    </div>
  );
};
