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

## Open before Greg's five-minute play gate

- [ ] **Frame rate.** The race runs at 3-4 vblanks per frame (15-20 fps); the
  title screen holds 2 (30 fps).  Measured cause: the Forge68 sprite renderer
  costs roughly 1500-2200 68000 cycles per hardware sprite column even when its
  cache suppresses every VRAM write, and a full-width road band is twenty
  columns.  See `05_PROGRAMMING_FOUNDATION/PERFORMANCE.md` for the measurements
  and the options.  This is the blocking issue: an arcade racer cannot be
  judged for feel at 15 fps.
- [ ] Road band count is currently six, chosen for frame rate rather than for
  looks.  The lateral step at a band boundary reaches roughly 37 px when the
  player is at the road edge.  More bands are wanted once the sprite path is
  cheaper.
- [ ] The HUD's lower row overlaps the player vehicle; recompose once the play
  area is settled.
- [ ] Title and character-select screens are placeholder text over the race
  backdrop.  `max-cruz-select-v2.png` is converted but not yet used.
- [ ] Audio is still the Forge68 SDK's driver and V-ROM content, not BAJANEW
  audio.  Audio direction is an open design decision.
- [ ] Expose BAJANEW's game-specific state through the enhanced Forge68 MCP
  host; do not count the SDK demo host as game evidence.
- [ ] Run Greg's five-minute unscripted MAME play gate before expanding scope.
- [ ] Do not produce the remaining stages until Greg approves the vertical
  slice.
