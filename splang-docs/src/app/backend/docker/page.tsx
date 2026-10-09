import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Box, Layers, Terminal, Server } from "lucide-react";

export const metadata = {
  title: "Static Compilation & Docker Scratch Containerization",
  description: "Generate static musl Linux binaries and build minimal scratch containers (< 15MB) in SP 3.0.",
};

export default function BackendDockerPage() {
  const dockerfileCode = `# Multi-Stage Ultra-Minimal Dockerfile
FROM debian:bookworm-slim AS builder

WORKDIR /build
# Install SP compiler toolchain
COPY . .

# Compile fully static self-contained binary linking musl libc
RUN spc build src/main.sp --static --release -o server

# Production stage: scratch (0MB base!)
FROM scratch

WORKDIR /app
COPY --from=builder /build/server /app/server

# Expose HTTP port
EXPOSE 8080

# Run native binary directly as PID 1
ENTRYPOINT ["/app/server"]`;

  const k8sManifestCode = `apiVersion: apps/v1
kind: Deployment
metadata:
  name: sp-backend-service
  labels:
    app: sp-backend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: sp-backend
  template:
    metadata:
      labels:
        app: sp-backend
    spec:
      containers:
      - name: sp-server
        image: registry.example.com/sp-backend:3.0.0
        ports:
        - containerPort: 8080
        resources:
          limits:
            memory: "64Mi"
            cpu: "500m"
          requests:
            memory: "16Mi"
            cpu: "100m"
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 2
          periodSeconds: 5
        readinessProbe:
          httpGet:
            path: /readyz
            port: 8080
          initialDelaySeconds: 1
          periodSeconds: 3`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 3.0 Backend Platform", href: "/backend/overview" },
          { title: "Static Compilation & Docker" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Box className="w-3.5 h-3.5" />
          <span>Cloud Native &bull; DevOps &amp; Containers</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Static Compilation &amp; Docker Scratch
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          Produce zero-dependency static binaries that link against musl libc. Package full-featured SP backends into bare-metal Docker <code>scratch</code> images under <strong>15 MB</strong> with zero runtime CVE attack surface.
        </p>
      </div>

      <Callout type="tip" title="Zero Shared Libraries">
        Running <code>spc build src/main.sp --static -o server</code> bundles all runtime routines and standard libraries into a single native binary. <code>ldd server</code> reports <code>not a dynamic executable</code>.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Production Multi-Stage Dockerfile</h2>
        <p className="text-xs text-slate-400 mb-3">
          Build the binary in a builder stage and package it onto bare <code>scratch</code>:
        </p>
        <CodeBlock code={dockerfileCode} filename="Dockerfile" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Kubernetes Deployment Manifest</h2>
        <p className="text-xs text-slate-400 mb-3">
          Deploy onto Kubernetes with configured CPU/memory limits, liveness probes, and readiness probes:
        </p>
        <CodeBlock code={k8sManifestCode} filename="deploy/k8s-deployment.yaml" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Container Comparison</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="border-b border-slate-800 text-slate-400">
              <tr>
                <th className="pb-2 font-sans font-semibold">Platform</th>
                <th className="pb-2 font-sans font-semibold">Container Image Size</th>
                <th className="pb-2 font-sans font-semibold">Base OS / Runtime</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 text-slate-300">
              <tr className="bg-cyan-950/20 text-cyan-300 font-bold">
                <td className="py-2 font-sans">SP 3.0 (Static musl)</td>
                <td className="py-2">12.5 MB</td>
                <td className="py-2">scratch (Zero OS overhead)</td>
              </tr>
              <tr>
                <td className="py-2 font-sans">Go (Static musl)</td>
                <td className="py-2">18.0 MB</td>
                <td className="py-2">scratch</td>
              </tr>
              <tr>
                <td className="py-2 font-sans">Rust (musl)</td>
                <td className="py-2">16.5 MB</td>
                <td className="py-2">scratch</td>
              </tr>
              <tr>
                <td className="py-2 font-sans">Node.js (Alpine)</td>
                <td className="py-2">140.0 MB</td>
                <td className="py-2">node:20-alpine</td>
              </tr>
              <tr>
                <td className="py-2 font-sans">Java / Spring Boot</td>
                <td className="py-2">280.0 MB</td>
                <td className="py-2">eclipse-temurin:21-jre</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Background Job Queues", href: "/backend/queue" }}
        next={{ title: "TechEmpower Benchmarks", href: "/backend/benchmarks" }}
      />
    </div>
  );
}
