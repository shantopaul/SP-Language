# ADR-005: Package Registry & Supply Chain Security

## Status
Proposed (Requires Shanto Review & Approval)

## Context
The SP Package Manager (`spm`) currently supports local package scaffolding, dependency resolution, and lockfile verification. The ecosystem requires a production-ready publishing and distribution model that guarantees supply chain integrity and build reproducibility.

## Options Considered
1. **Centralized Mutable Registry (Early npm style):** Packages can be modified or unpublished, causing ecosystem breakage.
2. **Decentralized Git-only Dependencies (Go style):** No central discovery, slow clone times, fragile tag tracking.
3. **Immutable Content-Addressed Central Registry with Local Offline Caching (Cargo / Hex style):** Cryptographic lockfile validation, immutable version releases, offline build caching.

## Decision
**Adopt Option 3 (Immutable Central Registry + `sp.lock` Cryptographic Integrity).**
- Packages published to `registry.splang.shantopaul.com` are immutable once released.
- Every release artifact includes a verified SHA-256 checksum recorded in `sp.lock`.
- `spm install --frozen` guarantees byte-for-byte reproducibility in CI/CD pipelines.
- Downloaded packages are cached locally in `~/.sp/cache/` for instant offline compilation.

## Consequences
- **Positive:** Maximum supply chain security; tamper-proof dependencies; fast offline builds.
- **Negative:** Requires cloud hosting for package tarball storage and edge metadata caching.
