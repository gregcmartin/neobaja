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

The Ensenada vertical slice runs on real Neo Geo hardware in MAME from the
approved artwork under `art/`: a twenty two band projected road on the SDK's
strip layers, a two layer parallax coast, 380 roadside props with crowds,
chevrons, signs and gantries, rivals that keep the leg a race, crest jumps,
roadside hazards, a shadowed HUD with 16x16 numerals and a route minimap, a
painted title and a results panel, and its own Z80 sound driver with an
original theme and a pitch-tracked engine.  The display holds a locked 30 Hz with
the simulation at its designed 60 Hz.  Greg's five-minute unscripted MAME
session remains the release gate; see `TODO.md` for what is open.

Verification commands:

- `make check` — deterministic simulation, sanitizer, 68000 compile, and trace.
- `make native-host` — run the Forge68 host renderer against the BAJANEW core.
- `make native-verify` — build and inspect the cartridge ROM.
- `make mame-smoke MAME_BIOS_DIR=/path/to/bios` — boot, telemetry, native PNG,
  and audio evidence in real MAME.
- `make mame-keyboard MAME_BIOS_DIR=/path/to/bios` — prove the tracked keyboard
  profile resolves A, B, 1, and arrow keys as intended.
- `make mame-controls MAME_BIOS_DIR=/path/to/bios` — prove neutral idle,
  throttle, coast, brake, and left/right behavior through emulated Neo Geo input
  fields.
- `make play MAME_BIOS_DIR=/path/to/bios` — launch the interactive build using
  the verified keyboard profile.

The enhanced Forge68 AI/MCP server is pinned with the SDK and recorded as the
game's planned structured test-control layer. Its SDK demo is not accepted as
BAJANEW evidence; a BAJANEW-specific state adapter remains a separate gate.
