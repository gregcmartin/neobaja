# BAJANEW clean-room rebuild packet

This workspace intentionally contains no game implementation, engine, tests,
build system, ROM, converted cartridge assets, MAME configuration, telemetry,
or runtime evidence.

Greg rejected the 2026-08-30 BAJANEW implementation and playability. Its game
art was approved and remains under `art/`; its source and executable artifacts
were deleted from the working tree. The rejection record is preserved under
`04_REJECTED_BAJANEW_ATTEMPT/` only as documentation of what not to repeat.

Start with `START_HERE.md`, then read `AGENTS.md`, the curated documents in
`01_GAME_VISION/`, and `02_REFERENCE_LIBRARY/README.md` before proposing a new
implementation.

## Preserved material

- `art/`: approved original raw game art, prompts, provenance, and hashes.
- `01_GAME_VISION/`: current game intent and human-first acceptance gates.
- `02_REFERENCE_LIBRARY/`: inspection-only visual and behavior references.
- `03_REJECTED_IMPLEMENTATION/`: rejection record for the older project.
- `04_REJECTED_BAJANEW_ATTEMPT/`: rejection record for the deleted BAJANEW
  implementation.
- `90_SOURCE_DOCUMENTATION_ARCHIVE/` and `99_MANIFESTS/`: historical packet
  records, not an implementation foundation.

No new game code should be written until these boundaries have been read.

## Current rebuild direction

The new implementation is programming-first. The pinned projects named in
`05_PROGRAMMING_FOUNDATION/REFERENCE_REGISTER.md` are nonshipping references;
BAJANEW's deterministic fixed-point road, handling, rival AI, collision, and
projection code is independently authored under `src/` and `include/`.

Run `make check` to execute the gameplay regression suite and generate a
deterministic telemetry trace. Native Neo Geo rendering and MAME validation are
the next gate; approved Baja art is integrated only after that gameplay layer
is controllable.
