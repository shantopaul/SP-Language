# SP Language — Ecosystem & Package Registry Strategy

**Document ID:** SP-ECO-001  
**Target:** `splang-registry`, `spm publish`, and Community Infrastructure  

---

## 1. Registry Architecture Overview

The SP Package Ecosystem is anchored by the central registry service at `https://registry.splang.shantopaul.com`.

```
Developer Workstation
      │
      ▼
 [ spm publish ]
      │  (HTTPS + API Token + Cryptographic SHA-256)
      ▼
 [ SP Registry Service ] (Next.js Edge API / Worker)
      ├── Metadata Storage (Versions, Dependencies, Authors, Readme)
      ├── Tarball Storage (S3 / Cloudflare R2: package-vX.Y.Z.tar.gz)
      └── Security Scanner (Dependency vulnerability & malware check)
           │
           ▼
 Developer Consumers (spm add / spm install)
```

---

## 2. Supply Chain Security Standard

Learning from modern supply chain risks in npm and PyPI:
1. **Immutable Package Versions:** Once a version `1.2.0` is published, it can never be altered or overwritten.
2. **Cryptographic Checksums in `sp.lock`:** Every dependency lock entry stores its SHA-256 hash. `spm install --frozen` enforces strict bit-for-bit integrity.
3. **Namespaces & Verified Scopes:** Packages with organization scopes (e.g. `@sp/http` or `@company/auth`) require verified publisher identity.
4. **No Arbitrary Post-Install Scripts:** Packages are strictly compiled by `spc`; no arbitrary shell scripts are executed during `spm install`.
