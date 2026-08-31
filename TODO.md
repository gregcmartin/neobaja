# Fresh clean-room rebuild gates

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
  snapshot (debug wireframe and development fixtures only).
- [x] Verify literal keyboard A/B/Start mappings and moving frames in MAME.
- [ ] Expose BAJANEW's game-specific state through the enhanced Forge68 MCP host;
  do not count the SDK demo host as game evidence.
- Reuse only the explicitly approved original artwork under `art/`.
- [ ] Build one Ensenada vertical slice focused first on readable road motion,
  responsive human controls, collision/off-road feel, and rival behavior.
- [ ] Run Greg's five-minute unscripted MAME play gate before expanding scope.
- [ ] Do not produce the remaining stages until Greg approves the vertical slice.
