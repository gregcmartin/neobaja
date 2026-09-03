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

- [x] Roadside dressed: 380 props read from the course, chevrons through the
  bends, crowds and gantries at both ends, lettered signs; objects placed
  through the SDK's sprite pool, budgeted by hardware columns.
- [x] HUD rebuilt: shadowed glyphs, 16x16 numerals for position, time and
  speed, a route minimap with the car's dot, and the rows the hardware shows.
- [x] Title with a typographic BAJA OUTRUN logo; a results panel at the
  finish with position, time, contacts and crashes.
- [x] Crests throw the vehicle at speed; roadside props are hazards off the
  road.

- [x] Closing on the mockups, 2026-09-03: props every seven metres with
  forty scenery columns and thirty-two draw items, dust banks either side of
  the player and behind near rivals, a sun over the coast (both new Grok
  Build raws), crest-thin bands merged, far props thinned past forty-eight
  metres.  Frame ~351k cycles, 4 of 700 frames over two fields; the tick-locked
  capture proves the C and assembly builds identical.

## Open before Greg's five-minute play gate

- [x] Grok Build art: painted title logo, checkpoint arch, billboard, pit
  awning, spectators, helicopter, boulders, drums, tyre stack and a distinct
  red-and-white rival buggy, all keyed and converted with provenance.

- [ ] 60 Hz needs about 35,000 more cycles a frame.  The general renderer's
  object path is the largest item; give objects fixed slot runs once the
  scene's object count is settled.
- [x] Sound: BAJANEW's own Z80 driver with a Timer-B sequencer, the
  "Pacific Run" theme, a pitch-tracked engine loop and effect samples.  See
  `05_PROGRAMMING_FOUNDATION/AUDIO.md`.  Mix balance and the tune itself await
  Greg's ear.
- [ ] Expose BAJANEW's game-specific state through the enhanced Forge68 MCP
  host; do not count the SDK demo host as game evidence.
- [x] Greg's unscripted MAME play gate, 2026-09-02: "amazing finally for the
  first time the game works looks good and actually plays properly."
- [ ] Remaining stages (San Felipe, Valle de Trinidad, Bahia de los Angeles)
  now unblocked by that verdict; confirm order and race structure with Greg.
