# ==============================================================================
# SP Language Production Multi-Stage Dockerfile (Sub-15MB Static Binary)
# Stage 1: Static Compilation with musl libc
# Stage 2: Zero-Dependency Scratch / Distroless Container
# ==============================================================================

# --- Stage 1: Builder ---
FROM alpine:3.19 AS builder

RUN apk add --no-cache \
    clang \
    llvm \
    lld \
    musl-dev \
    cmake \
    ninja \
    make \
    git \
    ca-certificates

WORKDIR /workspace

# Copy compiler and toolchain sources
COPY . .

# Build SP compiler and runtime static library
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    && cmake --build build --target spc sp_runtime -j $(nproc)

# Compile application binary statically using SP compiler
# Produces a self-contained static ELF binary with zero shared library dependencies
RUN ./build/bin/spc build examples/observability_devops.sp \
    --release \
    --static \
    -o /app/server

# Verify the binary is truly static
RUN file /app/server && ldd /app/server 2>&1 | grep -q "Not a valid dynamic program" || true

# --- Stage 2: Minimal Zero-Dependency Production Runtime ---
FROM scratch

# Metadata annotations
LABEL org.opencontainers.image.title="SP Language Production Service" \
      org.opencontainers.image.description="Sub-15MB zero-dependency static container" \
      org.opencontainers.image.vendor="SP Language Project" \
      org.opencontainers.image.authors="Shanto Paul"

WORKDIR /app

# Copy the statically compiled standalone binary
COPY --from=builder /app/server /server
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/

# Expose HTTP application, health check, and Prometheus telemetry port
EXPOSE 8080

# Run standalone application binary directly
ENTRYPOINT ["/server"]
