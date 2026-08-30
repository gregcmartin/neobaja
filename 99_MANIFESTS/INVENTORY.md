# BAJANEW inventory

Extraction date: 2026-08-30 (America/Chicago)

Source: `/Users/gregmartin/Desktop/BAJA Outrun`

Destination: `/Users/gregmartin/Desktop/BAJANEW`

## Curated contents

| Area | Files before manifests | Purpose |
| --- | ---: | --- |
| Root | 2 | Start guide and future-agent boundary |
| `01_GAME_VISION/` | 8 | Curated vision, acceptance, open decisions, and exact original request documents |
| `02_REFERENCE_LIBRARY/` | 22 | Four primary mockups, eight secondary screenshots, splash, and reference documentation |
| `03_REJECTED_IMPLEMENTATION/` | 16 | Nine negative-evidence screenshots and seven failure/history documents |
| `90_SOURCE_DOCUMENTATION_ARCHIVE/` | 126 | Exact non-build prose and machine-readable documentation archive |

The archive contains 88 Markdown files, 36 JSON files, one text checksum record,
and the old LICENSE. The JSON records are historical implementation metadata,
not approved design or production inputs.

## Image inventory

- 4 primary user mockups: PNG, 1448x1086.
- 8 secondary BAJA: Edge of Control HD images: JPEG, 1920x1080.
- 1 user-provided developer splash: JPEG, 1254x1254.
- 9 rejected MAME frames: PNG, native 320x224.

Total extracted size before hash manifest: approximately 15 MiB.

## Verification results

- Documentation archive checksum dry-run against the source selection: no
  differences.
- All four primary image hashes match `REFERENCE_MAP_ORIGINAL.md`.
- All eight secondary image hashes match their reference README.
- Developer splash hash matches its pinned SHA-256.
- Implementation leakage scan found no C/header/assembly, Python, Lua, object,
  ROM, Neo Geo ROM-part, or ZIP files.
- Source project remained in place; extraction was copy-only.

`FILES.sha256` hashes every BAJANEW file except the hash manifest itself.
