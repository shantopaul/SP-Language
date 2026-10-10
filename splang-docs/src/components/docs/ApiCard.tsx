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
    <div className="rounded-xl border border-[#0A1F44] bg-[#081735]/60 p-5 my-6 shadow-xl hover:border-[#0057FF]/50 transition-all">
      <div className="flex items-center justify-between gap-3 mb-2 flex-wrap">
        <h4 className="text-base font-bold text-white font-mono">{name}</h4>
        <span className="text-[11px] font-mono px-2 py-0.5 rounded-full bg-[#0057FF]/20 text-[#00D1FF] border border-[#00D1FF]/30 font-medium">
          Native Builtin
        </span>
      </div>

      <div className="bg-[#020D21] p-3 rounded-lg border border-[#0A1F44] font-mono text-xs text-[#00D1FF] mb-3 overflow-x-auto whitespace-pre">
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
          <div className="divide-y divide-[#0A1F44]/60 border border-[#0A1F44] rounded-lg overflow-hidden bg-[#020D21]/60">
            {parameters.map((param) => (
              <div key={param.name} className="p-2.5 text-xs flex flex-col sm:flex-row sm:items-center justify-between gap-1">
                <div className="flex items-center gap-2">
                  <span className="font-mono text-[#00D1FF] font-semibold">{param.name}</span>
                  <span className="font-mono text-slate-400 text-[11px]">({param.type})</span>
                  {param.optional && (
                    <span className="text-[10px] text-amber-300 px-1.5 py-0.2 rounded-full bg-amber-950/60 border border-amber-800">
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
          <code className="font-mono text-[#00D1FF] bg-[#020D21] px-2 py-0.5 rounded border border-[#0A1F44]">
            {returns}
          </code>
        </div>
      )}
    </div>
  );
};
