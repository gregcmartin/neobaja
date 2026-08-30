# Gauntlet scorecard

Round: rejected Codex native-MAME audit, 2026-08-30  
Critic identity: `codex-native-visual-20260830-final` (direct original-resolution image inspection)  
Outcome: **FAIL — prior 24/28 verdict withdrawn after live human play.** The
automation missed forced menu progression, idle auto-throttle, and inadequate
player-facing road feedback. Physical AES/MVS remains **UNVERIFIED**.

## Pixels opened

All four pinned primary mockups were opened at original resolution and used
only for comparison:

- `art/workbench/reference/user-neogeo-mockups/01-bahia-night-storm.png`
- `art/workbench/reference/user-neogeo-mockups/02-valle-canyon-jump.png`
- `art/workbench/reference/user-neogeo-mockups/03-san-felipe-desert-pack.png`
- `art/workbench/reference/user-neogeo-mockups/04-ensenada-pacific-run.png`

All eight secondary `art/workbench/reference/baja-edge-of-control-hd/01-*.jpg`
through `08-*.jpg` images were also opened. They informed density,
physicality, and terrain comparison only; no reference pixels ship.

Shipping review included the four canonical stage raws, the player and rival
raws/sheets, Max/Cruz, road, dust, logo, tach, minimaps, font, and splash. The
final native MAME review opened all 21 `build/evidence/baja-play-*.png` files:
two splash samples; title; Max/Cruz select; countdown; A/B motion pairs for
Ensenada, San Felipe, Valle, and Bahia; collision; off-road; every stage finish;
and next-stage.

## Scores

Scoring anchors: 0 absent/broken, 1 placeholder, 2 recognizable but materially
below target, 3 credible production Neo Geo translation with bounded gaps, 4
matches the intended result within the hardware.

| # | Category | Score | Native evidence |
| --- | --- | --- | --- |
| 1 | Composition, chase, depth, road | 3 | Large lower-third player RZR, perspective-scaled rivals, full-width shoulder/road plates, and animated foreground ruts form a clean chase view. The former black trapezoid and opaque prop-strip seams are absent in every final stage pair. The backdrop itself remains a fixed plate rather than a full raster road engine. |
| 2 | Player/rival scale and identity | 4 | The cobalt #8 RZR Pro R has cage, spare, shocks, and readable steer/contact/compression silhouettes. The red RZR and yellow Maverick remain visually distinct at live rival scale and are separately identified in telemetry. |
| 3 | Stage density and identity | 4 | Ensenada has Pacific cliffs, palms, helicopter, and crowd; San Felipe has open desert, mesa, cactus, spectators, and race sign; Valle has sunset canyon, drop, and chevrons; Bahia has moon, lightning, wet road, cacti, and checkpoint banner. Each final live plate is continuous and immediately identifiable. |
| 4 | Motion and physicality | 3 | Four foreground road frames advance, every stage A/B crop changes, rivals approach and change lanes, steering/contact/off-road poses read clearly, and dust/skid animation is live. Suspension and dust animation remain compact compared with the aspirational HD references. |
| 5 | Native pixel craft | 3 | Native 320x224 indexed output has hard alpha, legal 15-color banks, crisp nearest-neighbor clusters, populated C ROMs, and no road voids or rectangular extraction seams. Some distant environment clusters still reveal their downsampled-raster origin. |
| 6 | HUD, minimap, and type | 3 | POS 3/2/1 of 3, four-leg 1/4–4/4 progression, large cyan timer, dynamic AT1–AT5, KPH/tach, full stage names, best/lap time, and four minimaps remain readable during play. Terrain-backed labels and the compact tach are bounded presentation gaps. |
| 7 | Splash, title, select, race, finish | 4 | The pinned developer splash remains visible at both sampled times; title is centered and uncluttered; stable Max/Cruz selection frames identify both children and the sole vehicle; all four races, finishes, and next-stage flow render cleanly. |

**Prior total: 24/28 — WITHDRAWN. Overall result: FAIL.** Native stills met the
visual rubric, but a game that auto-advances and auto-drives without player
consent is not playable; still-image scoring cannot override that failure.

## Repair decision

No new bitmap generation was needed. Direct raw/source/native comparison showed
that the art itself was strong enough; the blockers were composition and
integration. The repair therefore retained the authenticated raws, removed the
oversized transparent road punch, reduced the foreground to one sparse
four-frame rut strip derived from the pinned rut raw, removed opaque extracted
prop rectangles that duplicated scenery already present in each authored
stage, and recentered the title/select FIX layout. This is deterministic native
pixel/conversion work, not relabeled generation.

## Gameplay and hardware evidence

The final virtual driver uses actual MAME input fields and records only a new
cartridge frame paired with a strictly newer VBlank. Its durable trace covers:

- Cruz selection plus left, right, throttle, and brake input;
- on-road maximum 26 versus off-road maximum 13, with recovery;
- brake loss 2 over 12 frames versus coast loss 1 over 16 frames;
- blocking RZR and evasive Maverick approach, distinct lane response, both
  passes, and place 3/2/1;
- collision and non-collision running frames;
- all four 1,000-plus-frame race legs, finishes, and next-stage flow;
- all four road frames and native foreground pixel motion on every stage;
- zero dropped columns, zero dropped commands, zero overloaded scanlines, with
  83 maximum active columns and a 59/96 peak scanline load.

The authoritative machine-readable artifact is
`build/evidence/mame-play-summary.json`; the underlying sequential record is
`build/evidence/mame-play-trace.jsonl`. Two consecutive clean `make check`
builds produced the same `puzzledp.zip` SHA-256:
`9bbd283e4fd03be9e99f792b3f6e6f8950760f2d19702956adb50629474dea6a`.
Physical AES/MVS timing, controllers, video, and long-soak behavior are not
inferred from MAME and remain unverified.
