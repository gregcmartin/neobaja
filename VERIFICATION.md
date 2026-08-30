# Ensenada vertical-slice verification

Verified on 2026-08-30 with the installed ngdevkit 0.5 toolchain, GCC 15.3,
and MAME 0.289.

## Automated results

- `make test`: PASS — deterministic input gates, idle behavior,
  throttle/coast/brake, steering/recentering, independent rival movement,
  off-road penalty/recovery, collision loss, overtake/order, finish/restart.
- `make production-check`: PASS — pinned raw and converted art hashes,
  developer splash, font provenance, and deterministic conversion.
- `make verify`: PASS — cartridge members and budgets; 2,428 sprite tiles.
- `make controls-check`: PASS — Enter, arrow keys, literal keyboard A for
  confirm/throttle, and literal keyboard B for brake.
- `make mame-smoke`: PASS — 16 native 320x224 AES/MVS captures covering boot,
  input-gated menus, countdown/rest, acceleration, steering, coast, rival,
  contact, off-road response, and the MVS coin/start path.

ROM: `build/rom/bajanew.zip`

SHA-256: `29023d714d8d522414cecc8c0de0b8ca93e86de4ceb0c76f227c619423f2350f`

68000 ELF footprint: 12,512 bytes text, 108 data, 631 BSS.

## Gate still requiring Greg

This file does not mark the game approved. The five-minute unscripted MAME
session in `01_GAME_VISION/PLAYABILITY_AND_ACCEPTANCE.md` remains mandatory,
and physical AES/MVS hardware remains untested.
