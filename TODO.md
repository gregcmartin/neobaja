# Ensenada vertical slice status

- Read `START_HERE.md`, `AGENTS.md`, `01_GAME_VISION/`, and
  `02_REFERENCE_LIBRARY/README.md`.
- [x] Choose and document an independent fixed-point implementation approach
  without recovering anything from the rejected BAJANEW implementation.
- [x] Pin Cannonball, the OutRun arcade SDK article, and Pseudo3D-road as
  nonshipping programming references.
- [x] Select the enhanced Forge68 SDK and document its MCP host/MAME testing
  loop, neutral-input requirement, and BAJANEW art-policy override.
- [x] Pass the deterministic host gameplay and road-projection suite.
- [x] Build a fresh Neo Geo renderer/input shell around the immutable simulation
  snapshot.
- [x] Verify literal keyboard A/B/Start mappings and moving frames in MAME.
- [x] Convert the approved original artwork under `art/` into the cartridge:
  road bands rectified from the Ensenada plate, player and rival vehicles,
  scenery, dust, the developer splash, and a hand-authored HUD font.
- [x] Draw a real projected road, rivals, scenery and HUD on Neo Geo hardware
  in MAME.

- [x] Optimise the Forge68 sprite path.  The race now holds a locked 30 Hz
  display with the simulation advancing at its designed 60 Hz, up from 3-4
  vblanks per frame with the simulation running at a third speed.  Measurements
  and method are in `05_PROGRAMMING_FOUNDATION/PERFORMANCE.md`; the flush's
  equivalence to the unoptimised path is checked by `make renderer-check`.

## Open before Greg's five-minute play gate

- [ ] Road band count is eight, still chosen partly for frame rate.  The
  lateral step at a band boundary reaches roughly 26 px when the player is at
  the road edge.  Ten bands cost 2.13 vblanks and were backed out.
- [ ] 60 Hz would need about another 125,000 cycles a frame.  The flush is
  still the largest item; precomputing each frame's SCB1 words at asset compile
  time is the next thing worth measuring.
- [ ] The HUD's lower row overlaps the player vehicle; recompose once the play
  area is settled.
- [ ] Scenery still reads dark at 1x at long range; the chevron sign in
  particular wants more contrast against the dirt.
- [ ] Title and character-select screens are placeholder text over the race
  backdrop.  `max-cruz-select-v2.png` is converted but not yet used.
- [ ] Audio is still the Forge68 SDK's driver and V-ROM content, not BAJANEW
  audio.  Audio direction is an open design decision.
- [ ] Expose BAJANEW's game-specific state through the enhanced Forge68 MCP
  host; do not count the SDK demo host as game evidence.
- [ ] Run Greg's five-minute unscripted MAME play gate before expanding scope.
- [ ] Do not produce the remaining stages until Greg approves the vertical
  slice.
