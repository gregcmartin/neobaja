# Gauntlet scorecard

Round: post-HUD critic (fresh harsh visual+functional after artifact-changing build)
Critic identity: `gauntlet-critic-visual-20260830-1314` (image-capable `read_file`)
Outcome: **FAIL**. Visual **17/28** (need every category >=3 and total >=24). Functional gates mostly green. Physical AES/MVS **UNVERIFIED**.

## Paths opened (image-capable)

Primary mockups (byte-for-byte pinned, inspection only; hashes match README):

- `art/workbench/reference/user-neogeo-mockups/01-bahia-night-storm.png` SHA-256 `9f39c438479e9a6501ccfeac6fb1f408269a39c74e0803bfbae338ffb9843cda`
- `art/workbench/reference/user-neogeo-mockups/02-valle-canyon-jump.png` SHA-256 `1f96f3a06c00cea5c563406bf532d656a271f5c266a032571929e1a2602d029e`
- `art/workbench/reference/user-neogeo-mockups/03-san-felipe-desert-pack.png` SHA-256 `f444fde3a1063354d9936842bc8518de49c6263d8730f2da54e341f282777a81`
- `art/workbench/reference/user-neogeo-mockups/04-ensenada-pacific-run.png` SHA-256 `22f162a5c8e2132e4b75dde95830fae1ba2c58b4cb488f2159e9e16a48308b7c`

Also read `art/workbench/reference/user-neogeo-mockups/README.md` and `art/workbench/reference/baja-edge-of-control-hd/README.md`.

Secondary HD refs (hashes match pack README):

- `art/workbench/reference/baja-edge-of-control-hd/01-race-dust.jpg`
- `art/workbench/reference/baja-edge-of-control-hd/02-race-pack.jpg`
- `art/workbench/reference/baja-edge-of-control-hd/03-terrain.jpg`
- `art/workbench/reference/baja-edge-of-control-hd/04-canyon.jpg`
- `art/workbench/reference/baja-edge-of-control-hd/05-jump.jpg`
- `art/workbench/reference/baja-edge-of-control-hd/06-landscape.jpg`
- `art/workbench/reference/baja-edge-of-control-hd/07-hillclimb.jpg`
- `art/workbench/reference/baja-edge-of-control-hd/08-terrain-variation.jpg`

Shipping rasters / raws opened: `devsplashlogo.jpg`, `assets/source/{splash,player_rzr,max,cruz,ai_rzr,ai_maverick,desert_{ensenada,san_felipe,valle,bahia},horizon_{ensenada,san_felipe,valle,bahia},road,font,hud_minimap,hud_tach,logo,sign,dust}.png`, `art/raw/{player_rzr_canonical,stage_ensenada_canonical,stage_valle_canonical,stage_bahia_canonical,stage_sanfelipe_canonical}.jpg`.

Fresh native 320x224 MAME captures (this critic re-opened after `MAME_BIOS_DIR=/Users/gregmartin/Desktop/goneo make mame-smoke`):

- `build/evidence/baja-splash-mame.png` / `baja-splash-a.png` / `baja-splash-b.png`
- `build/evidence/baja-title-mame.png` / `baja-title.png`
- `build/evidence/baja-select-mame.png` / `baja-select.png`
- `build/evidence/baja-mame.png`
- `build/evidence/baja-ensenada-a.png` / `baja-ensenada-b.png`
- `build/evidence/baja-san-felipe-a.png` / `baja-san-felipe-b.png`
- `build/evidence/baja-valle-a.png` / `baja-valle-b.png`
- `build/evidence/baja-bahia-a.png` / `baja-bahia-b.png`
- `build/evidence/baja-finish.png`
- `build/evidence/baja-next-stage.png`

## Scores (0-4)

Scoring anchors: 0 absent/broken, 1 placeholder, 2 recognizable but materially below, 3 credible production Neo Geo translation with bounded gaps, 4 matches intent within hardware.

| # | Category | Score | Visible evidence |
| --- | --- | --- | --- |
| 1 | composition, chase, depth, road | 2 | Rear-chase and 112x96 blue RZR sit in the lower third with a grooved funnel, but every race/title/select frame has a large black trapezoid through the racing line (`punch_road_funnel` in `tools/convert_baja_art.py`). ~12% of Ensenada race pixels are RGB sum <12. Mockups fill dirt edge-to-edge. Shoulder tiles never meet the projected road. |
| 2 | player/rival scale and identity | 3 | Live cobalt RZR Pro R has cage, spare, shocks, light bar, #8. Yellow Maverick vs red RZR are distinct silhouettes. Scale occupies the lower third but is below mockup trophy-truck mass. Mid-pack rivals smear into a quantized blob. |
| 3 | stage density and identity | 2 | Horizons read: Ensenada ocean/heli/palms; San Felipe mesa/cactus/`SAN FELIPE DESERT DASH`; Valle canyon/sun/chevrons; Bahia moon/lightning. Same punched-road geometry and rival layout on all four. Jump/checkpoint/pack events are missing in live frames. Raws are denser than the converted playfield. |
| 4 | motion / physicality | 2 | Stage A/B pairs are not stills (mean abs 4.9–10.6; KPH 060 vs 073; ruts shift). Dust is a tiny 16x16 puff. Smoke telemetry pose stays 0. No readable steer/compression/airborne in the captured pairs. HD refs show airborne cars, tire dust, and suspension travel. |
| 5 | native pixel craft | 2 | Hardware indexing is legal (opaque_colors=15, soft_alpha=0, C 95424/1048576, peak scanline 54/96). Playfield still reads as 15-color posterized Imagine JPEGs (desert RMS 13–19, max error 92–130) plus hard black punch seams, not authored SNK clusters. FIX HUD type is the exception. |
| 6 | HUD / minimap / type | 3 | Mockup hierarchy is on 320x224: POS/LAP UL, outlined cyan TIME UC, BEST/LAP TM UR, tach+KPH+AT LL, full stage names LR (`ENSENADA`; `SAN FELIPE`; `VALLE DE TRINIDAD`; `BAHIA DE LOS ANGELES`) plus 64x48 circuit minimap. Gaps: TIME is native 16 not mockup-huge; names sit on terrain; tach is a quantized photo gauge; minimaps are outline-only. |
| 7 | splash/title/select/race/finish | 3 | Splash is the letterboxed Max Cruz Racing Team JPEG (15 colors, identical a/b/mame). Title/select show Max jersey 8 blond and Cruz jersey 6 brown plus `2022 POLARIS RZR PRO R`. Four live stages, `FINISH`, `NEXT STAGE`. Title/select inherit the black funnel. Helicopter overlaps Cruz. |

**Total: 17/28**. Visual PASS requires >=24/28 with every category >=3. Categories 1, 3, 4, and 5 are 2.

## Lowest gap / repair

Lowest remaining: composition (cat1=2) tied with native pixel craft (cat5=2). The desert conversion punches a transparent trapezoid that the projected road does not fill, so live MAME shows a black hole through the racing line on all four stages. Repair: stop punching (or shrink the funnel to the actual road sprite coverage), fill shoulders with opaque dirt from the hashed stage raws, and recraft converted playfield/vehicle sheets toward hard Neo Geo clusters. Do **not** relabel fixtures. Do not resize mockups into C-ROM.

## Functional gates (this critic)

- `python3 tools/grok_art.py preflight --require-auth` ready=true (grok 1.0.13, grok-4.6)
- `python3 tools/grok_art.py audit --profile production` passed=true; grok_build raw SHA-256s match files; session `01a05371-ecfe-75c3-9686-b374741dc1da` exists under `~/.grok/sessions/`
- `make production-check` PRODUCTION CHECK PASS
- `make test` ENGINE/BAJA HOST TEST PASS + 11 Python tests; splash active 0 and 299, title at 300; MAX/CRUZ; `2022 POLARIS RZR PRO R`; distinct rival ids; four named stages; `hud_stage` contains ENSENADA
- `make verify` ROM VERIFY PASS; P1 `7ba27c92de64b72505b4d12fa8cea8752aae9821a9d9490e6bf59198a74ce7b5` / S1 `2ee333826090794fb0d308b53e79e384e239e5d42785ecaf539a81ec08e49090` / C1 `d581390fdfc460e555dea93e6ea991c106c6c24558f74e8aa23c2b9bc545ee00`; layout P1 512KiB S1 128KiB M1 128KiB V1 512KiB C1/C2 1MiB; no `neogeo.zip` in tree
- `make docs-check` after this scorecard
- External-BIOS `MAME_BIOS_DIR=/Users/gregmartin/Desktop/goneo make mame-smoke` `MAME_SMOKE_PASS frames=520 columns=138 peak=54 overloaded=0 dropped_columns=0`; 320x224, 156 colors
- Splash JPEG SHA-256 `6e01d4f3fdb9daaa6feb90b52ab0497e3f26b5191c6ac205b0efeed1ed6eeba1`
- C-ROM is populated (c1 nonzero 67832 bytes of 1MiB); `game/baja.c` submits horizon/desert/road/player/rivals/dust/HUD FIX
- This critic did not destroy `build/evidence/` to repeat `make clean && make rom`; current ROM hashes match `build/evidence/rom_sha256.txt`
- Physical AES/MVS: **UNVERIFIED**
