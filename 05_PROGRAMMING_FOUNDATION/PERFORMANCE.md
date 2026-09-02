# What the 68000 can afford

Every number here was measured on the real cartridge in MAME. The game writes a
stage number to `bajanew_stage` at each step of its frame; a MAME script taps
that address and turns the gaps into 68000 cycles, which is the only way to see
costs smaller than a whole 60 Hz field. `bajanew_render_level` peels layers off
the scene so each one can be priced separately. Nothing in the game writes
either byte.

## Where it started

The Ensenada race ran at 3.1 fields per frame, and because the simulation
stepped once per drawn frame it was also running at roughly a third of the
sixty steps a second everything is tuned for. A frame cost about 472,000
cycles against a field's 200,000.

## Where it is now

Racing holds **2.01 fields per frame - a locked 30 Hz display - with the
simulation advancing twice per drawn frame, so gameplay runs at its designed
60 Hz.** The road carries eight bands rather than the six it had been cut to
for speed.

Frame breakdown while racing, in 68000 cycles:

| Stage | Before | After |
| --- | ---: | ---: |
| simulation step | 18,720 | 19,700 (two steps now) |
| FIX clear and renderer begin | 16,904 | 6,100 |
| view and road band projection | 54,063 | 45,000 |
| backdrop and road submit | 19,676 | 22,000 |
| scenery, rivals, player, HUD submit | 118,833 | 61,000 |
| renderer flush | 220,207 | 140,000 |
| FIX flush | 24,208 | 10,400 |
| **total** | **472,612** | **~305,000** |

## What the renderer cost, and why

A sprite column cost 1,500 to 2,200 cycles **even when the renderer's cache
suppressed every VRAM write** - cartridge telemetry reported `vram_writes = 0`
on a still frame that still took six fields to draw. It was not bandwidth. It
was rebuilding each column's tile pointer, zoom and control words from scratch
every frame, for every column, including the twenty a full-width road band
needs.

The flush now remembers each command's run of hardware sprites. If a command
lands on the same sprites with the same tiles, palette and transform as last
frame, every chained column after the head is provably unchanged: only the head
carries a position, and the sticky columns inherit it. A wide object that slid
sideways costs one word. One that only resized costs one word per column,
because shrink is per hardware sprite, and touches no tile map.

Reuse is only sound while a run still owns its sprites. The sort order changes
as objects move, so the flush records which command last wrote each hardware
sprite and refuses to reuse a run that has been overwritten.

Scanline pressure moved from adding to every covered line - which grew with the
total height of every wide object - to two edits per command drained once, in
place, so the next frame starts clean without a separate clear. That alone was
43,000 cycles a frame.

## Everything else that was in the way

- **Stale objects.** `native/Makefile` had no header dependencies, so a changed
  struct layout linked old and new objects together and the game read its own
  state at the wrong offsets. `-MMD -MP` now tracks headers.
- **64-bit fixed point.** `baja_fp_mul` went through libgcc's `__muldi3` plus a
  64-bit shift. It is now four 16x16 products, inlined in the header - out of
  line, the call and register save cost more than the arithmetic.
- **Hot divisions.** The segment length is a power of two, the object scale is
  one 32-bit divide, the speed ratio is a constant multiply, and the HUD's
  decimal conversion uses exact 16-bit reciprocals instead of two software
  divisions per digit.
- **One view per frame** instead of rebuilding the road-tangent frame per
  object.
- **FIX layer.** Only rows the HUD wrote are touched, and each row is cleared
  and compared four cells at a time.
- **Backdrop.** Sky and ground were separate full-width layers, forty columns
  between them; they are one panorama now.
- **Camera shake** is a uniform pixel offset rather than a term in the
  projection, so a band's height - and therefore its shrink - does not change
  every frame.
- **The road is contiguous in the sprite table.** Objects used to be
  interleaved between the bands they stand on, so an object changing band
  shifted every later band onto new hardware sprites.
- The 68000 target builds at `-O2`; ROM is 41 KB of 512 KB.

## Proving it changed nothing

`tools/verify_renderer.py` builds the game's host renderer twice, once with the
fast paths and once with `NG_RENDERER_NO_RUN_REUSE`, and compares the VRAM
digest after every frame of an identical 1500 frame run. They match exactly.
The SDK's own `test_wide_object_reuse` pins the write counts: a chained
sixteen column object costs zero words when nothing moved, one when it slid,
one per column when it resized, and a full rebuild when its frame changed.

## The strip layer road (2026-09-02)

The road and backdrop no longer go through the general renderer at all.  A
band is a sticky chain of hardware sprites in a fixed run of slots, and the
SDK's strip layer writes only the words that changed: three for a band that
moved, a tile map per column when its window slides across a sixteen pixel
boundary, and sixteen palette words when its surface phase flips.  That let
the road grow from eight bands to twenty two, with the near bands capped at
twelve rows so a strip's inability to shear stays under a few pixels, and it
took the renderer flush from 140,000 cycles to under 40,000.

Racing, in 68000 cycles, with the twenty two band road:

| Stage | Cycles |
| --- | ---: |
| simulation step (two steps) | 19,600 |
| FIX clear and renderer begin | 6,000 |
| view and road band projection | 35,200 |
| backdrop and road strips | 61,300 |
| scenery, rivals, player submit | 49,800 |
| renderer flush | 39,300 |
| FIX flush | 10,800 |
| **total** | **~222,000** |

The projection walks the segments ahead once per frame and interpolates each
band boundary with single 16 by 16 multiplies.  The strips' cost is almost all
per-band glue: a placement that changes nothing still costs about 900 cycles,
and the surrounding loop about as much again, because that is what 16-bit C
costs on this machine.

## What would buy the next field

60 Hz now needs about 35,000 cycles.  The general renderer's object path is
the largest remaining item at roughly 870 cycles per column; giving each
object a fixed run of slots the way the road has would remove the sort and
the run bookkeeping.  That is worth doing once the scene's object count is
settled, not before.

## Earlier: what would have bought the next field

60 Hz needs the frame under about 180,000 cycles, which is another 125,000.
The flush is still the largest item at roughly 140,000, and about 50 of the
190 columns are genuinely rebuilt each frame because the objects that move also
scale, change level of detail, and reorder. Precomputing each frame's SCB1
words at asset compile time would turn a tile map upload into a copy, and is
the next thing worth measuring.
