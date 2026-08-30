# Verification

| Command | Gate |
| --- | --- |
| `python3 tools/convert_baja_art.py` | Deterministically rebuild native source sheets from pinned raws |
| `make production-check` | Authenticated raw/source hashes, hard alpha, approved provenance, no fixtures |
| `make test` | Host engine, gameplay relationships, art policy, scenes, and deterministic replay |
| `make rom` | Build and pack P/S/M/V/C ROMs plus `puzzledp.zip` |
| `make verify` | ROM ABI, layout, capacity, tile use, and SHA-256 evidence |
| `make docs-check` | Required public docs and local-link integrity |
| `MAME_BIOS_DIR=... make mame-smoke` | Native boot, telemetry, 320x224 rendering, and scanline budget |
| `MAME_BIOS_DIR=... make mame-play` | Input-driven, frame-synchronous complete virtual playthrough |
| `MAME_BIOS_DIR=... make mame-evidence` | Alias for `mame-play` |

The virtual-play gate uses MAME's emulated Neo Geo input fields, not direct
simulation mutation. It requires consecutive cartridge frame counters and a
strictly newer game VBlank for every trace row. The JSON validator rejects:

- missing title, Max/Cruz select, countdown, running, finish, or next-stage
  states;
- a race leg shorter than 1,000 live gameplay frames;
- absent throttle, brake, left, or right input;
- missing collision/non-collision, off-road/recovery, steering/recentering,
  brake-over-coast, place 3/2/1, rival approach/pass/lane response, or distinct
  blocking/evasive profiles;
- non-advancing four-frame foreground road animation, identical A/B frames, a
  near-black center-road void, or insufficient foreground pixel motion;
- dropped renderer work or more than 96 active sprite columns on a scanline.

Evidence is written to `build/evidence/mame-play-trace.jsonl`,
`mame-play-summary.json`, `mame-play.log`, and `baja-play-*.png`. All visual
scores must be based on opening the native PNG pixels, not filenames or hashes.

BIOS files must be supplied externally. Never copy `neogeo.zip` into this tree.
Physical AES/MVS boards remain unverified until tested on actual hardware.
