# backend-splang

High-performance enterprise REST API service built with SP Language.

## Development

Start the auto-reloading development server:
```bash
spm dev
```

## Running Tests

Execute the in-memory integration test suite without socket binding:
```bash
spm test
```

## OpenAPI 3.0 Documentation

Generate OpenAPI 3.0.3 specification and Swagger UI documentation:
```bash
spc doc src/main.sp --swagger -o docs
```

## Production Container Build

```bash
docker build -t backend-splang .
docker run -p 8080:8080 backend-splang
```
