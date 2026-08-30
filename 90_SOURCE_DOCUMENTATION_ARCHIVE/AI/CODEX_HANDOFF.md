# Codex completion handoff

The UltraGrok workflow is archived and cannot resume. BAJA Outrun no longer has
an open software release blocker: the native MAME visual score is 24/28 and the
frame-synchronous gameplay oracle passes. Treat `AI/CURRENT_STATE.md` and
`docs/GAUNTLET_SCORECARD.md` as the current state; older handoff scorecards are
historical.

## Reproduce the release candidate

```sh
python3 tools/convert_baja_art.py
make production-check
make test
make rom
make verify
make docs-check
MAME_BIOS_DIR=/path/to/neogeo-bios make mame-smoke
MAME_BIOS_DIR=/path/to/neogeo-bios make mame-play
```

The BIOS stays external. Do not run Grok preflight or generation. The current
art repair is deterministic conversion/integration from the existing pinned
raws; no OpenAI bitmap was generated and no historical provenance was
relabelled.

## Evidence authority

- `build/evidence/mame-play-summary.json` — validated proof summary.
- `build/evidence/mame-play-trace.jsonl` — every consecutive game frame and
  unique VBlank through the complete scripted drive.
- `build/evidence/baja-play-*.png` — 21 native 320x224 visual checkpoints.
- `docs/GAUNTLET_SCORECARD.md` — paths opened, category scores, and remaining
  bounded visual gaps.

`mame-play` uses the real emulated controls and rejects short race legs,
missing mechanics, static stage pairs, near-black road voids, dropped render
work, and incomplete lifecycle coverage. It selects Cruz; Max is separately
captured on the stable selection screen and covered by host tests.

## Remaining boundary

Only physical AES/MVS verification and optional post-release scope remain.
Actual hardware testing must record board/BIOS identity, controller behavior,
audio/video timing, and soak results before changing `UNVERIFIED` to `PASS`.

The linked OutRun repository remains a read-only GPL-3.0 behavior oracle. Copy
no source, constants, data, maps, art, or trade dress into this MIT project.
