# BAJANEW restart packet

BAJANEW is a documentation-and-reference handoff for restarting BAJA Outrun.
It is not a game build and does not contain a ROM, engine, source code, build
system, converted shipping art, or emulator BIOS.

## Current reset state

Greg also rejected the 2026-08-30 BAJANEW implementation after live play: its
art was good, but its implementation and playability were unacceptable. That
implementation has been deleted. Its documentation-only rejection record is
under `04_REJECTED_BAJANEW_ATTEMPT/`.

The original raw artwork under `art/` is explicitly approved to carry forward.
Nothing else from the rejected BAJANEW implementation may carry forward:
source, engine behavior, constants, architecture, tests, tools, build files,
ROMs, converted assets, emulator configuration, runtime evidence, measurements,
and automated scores are all excluded.

The previous implementation was rejected in live play. Its automated checks
proved that code paths and telemetry changed, but they did not prove that the
game felt playable or that the road looked like a moving race course. That
release verdict is invalid.

## Read in this order

1. `01_GAME_VISION/GAME_VISION.md` — the requested game, cleaned of old build
   assumptions.
2. `01_GAME_VISION/PLAYABILITY_AND_ACCEPTANCE.md` — the human-first definition
   of playable.
3. `01_GAME_VISION/OPEN_DESIGN_DECISIONS.md` — choices that should be made with
   Greg rather than guessed.
4. `02_REFERENCE_LIBRARY/README.md` — what each reference may and may not be
   used for.
5. `03_REJECTED_IMPLEMENTATION/WHAT_FAILED.md` — lessons from the rejected
   build, not a design foundation.
6. `04_REJECTED_BAJANEW_ATTEMPT/WHAT_FAILED.md` — Greg's newest live verdict
   and the clean-room boundary for the deleted BAJANEW attempt.

## Authority order

When documents disagree, use this order:

1. Greg's explicit current direction and the original user requests.
2. The four user-supplied Neo Geo mockups for visual intent.
3. The clean-room gameplay relationships in the behavior oracle.
4. The BAJA: Edge of Control HD images for secondary physicality and terrain
   observations only.
5. Rejected implementation documentation only as a list of mistakes to avoid.

The complete prior prose and machine-readable records are preserved under
`90_SOURCE_DOCUMENTATION_ARCHIVE/`. They are historical evidence, not current
instructions. In particular, old PASS claims, build commands, implementation
constants, generated-art prompts, and provenance records are not approved for
reuse.

## Semi-clean-room boundary

New work may carry forward the game concept, explicit character and vehicle
identity, stage names and moods, inspection-only reference images, relational
gameplay behavior, and documented failure lessons. It must not carry forward
the rejected source code, engine adaptation, constants, art raws, converted
assets, ROMs, test harness, telemetry driver, or automated score.

For the deleted BAJANEW attempt specifically, the only implementation-era
material approved to carry forward is the original artwork under `art/` and
the written failure lesson. Do not recover its game code from Git history.

The safest restart is one playable Ensenada vertical slice first: title and
selection that wait for the player, a genuinely moving projected road, one
responsive RZR, one independently moving rival, collision/off-road feedback,
and a five-minute human MAME play session. Do not expand to four stages until
Greg approves that slice.

## Folder map

- `01_GAME_VISION/`: current design intent and acceptance gates.
- `02_REFERENCE_LIBRARY/`: unchanged inspection references and provenance.
- `03_REJECTED_IMPLEMENTATION/`: quarantined screenshots and failure records.
- `04_REJECTED_BAJANEW_ATTEMPT/`: documentation-only record of the deleted
  BAJANEW implementation and Greg's live rejection.
- `art/`: approved original artwork, prompts, provenance, and hashes.
- `90_SOURCE_DOCUMENTATION_ARCHIVE/`: exact documentation archive from the
  rejected project, excluding build output and NVRAM.
- `99_MANIFESTS/`: hashes, inventory, and explicit extraction exclusions.
