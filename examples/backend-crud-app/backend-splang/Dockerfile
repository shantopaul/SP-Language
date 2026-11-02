# Multi-stage minimal static container build for SP Language
FROM debian:bookworm-slim AS builder
WORKDIR /app
COPY . .
RUN spc build src/main.sp --static --release -o server

FROM scratch
WORKDIR /app
COPY --from=builder /app/server /app/server
EXPOSE 8080
ENTRYPOINT ["/app/server"]
