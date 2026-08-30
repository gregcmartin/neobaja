# Current state

Initial Grok handoff: **2026-08-30 13:18:36 CDT**  
Codex release-candidate repair: **2026-08-30**

## Outcome

The prior 24/28 release-candidate verdict was rejected by live human play and
is withdrawn. The automated path injected inputs immediately and missed forced
menu progression plus idle auto-throttle; it also overvalued clean stills over
convincing road motion. Human-first repair is in progress. Physical AES/MVS
hardware is still **UNVERIFIED**.

The archived UltraGrok workflow remains `interrupted` and must not be resumed.
No Grok login or live process is required. Historical Grok raws and provenance
remain build inputs verified offline by hash.

## Codex repairs

- Created an external rollback archive before broad changes.
- Removed the transparent road punch that caused a black trapezoid through all
  four playfields.
- Rebuilt the road overlay as sparse two-color, hard-alpha, four-frame rut
  detail from the authenticated rut raw and limited it to the stable nearest
  Neo Geo strip.
- Removed opaque extracted-prop overlays that produced rectangular scenery
  seams; the full authored stage plates already contain the intended dressing.
- Recentered and decluttered title/select screens and stabilized selection
  capture timing.
- Added real off-road drag/speed cap, speed-dependent curve force, progressive
  lateral response, dynamic AT1–AT5, truthful 3-car place and 1/4–4/4 leg HUD,
  longer race legs, and competitive rival speeds.
- Added distinct blocking RZR and evasive Maverick profiles with lane targets.
- Expanded host regression coverage for off-road/recovery, curves, rival
  profiles/speeds, race length, gear, place, and leg feedback.
- Expanded the reserved 64-byte telemetry block and added actual MAME input
  driving, sequential game-frame/VBlank logging, automated visual captures,
  road-void/motion checks, and a complete four-stage oracle validator.

## Authoritative gates

Use the sequence in `docs/VERIFICATION.md`. `make mame-play` is the definitive
live software gate and writes `build/evidence/mame-play-summary.json`, the full
JSONL trace, MAME log, and 21 native PNGs. `make mame-evidence` aliases it.

The final score and inspected paths are in `docs/GAUNTLET_SCORECARD.md`.
Physical-hardware soak and optional expansion work are listed in `TODO.md`.

## Preserved boundaries

The pinned OutRun checkout remains a GPL-3.0 behavior-only oracle. No upstream
source, constants, data, assets, maps, or trade dress were copied. No new bitmap
generation was needed, so no provenance provider was changed or relabeled.
