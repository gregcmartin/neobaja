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

- [x] Road rebuilt on the SDK's strip layers: twenty two bands, per-band
  palettes carrying the surface phase and depth haze, camera tracking the
  player at sixty percent so the near bands' shear error stays under a few
  pixels, and a two layer parallax backdrop cut from one scaling of the plate.

## Open before Greg's five-minute play gate

- [ ] 60 Hz needs about 35,000 more cycles a frame.  The general renderer's
  object path is the largest item; give objects fixed slot runs once the
  scene's object count is settled.
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
