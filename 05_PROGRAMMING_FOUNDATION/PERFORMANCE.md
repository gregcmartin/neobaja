# What the 68000 can afford

Every number here was measured on the real cartridge in MAME by writing a
render level into `bajanew_render_level` and reading the cartridge's own
telemetry: game frames against vblanks, so the metric is exactly "how many
60 Hz fields does one game frame take".

## Method

`native/game.c` peels the frame apart by level, and nothing in the game ever
writes that byte:

| Level | Frame contains |
| ---: | --- |
| 7 | simulation and main loop only, no drawing |
| 6 | + FIX clear/flush and an empty renderer flush |
| 5 | + the phase's HUD and menu text |
| 4 | + the shared view and the road band projection |
| 3 | + the backdrop panorama |
| 2 | + the road bands |
| 1 | + scenery |
| 0 | + rivals, player, dust |

Levels 16 and up draw only the first N road bands, which produces a cost curve
against hardware sprite columns.

## Results

Held at the title screen, six bands, after the optimisations below:

| Level | Columns | Vblanks per game frame |
| ---: | ---: | ---: |
| 7 | – | 1.000 |
| 6 | 0 | 1.000 |
| 5 | 0 | 1.000 |
| 4 | 0 | 1.000 |
| 3 | 20 | 1.003 |
| 2 | 111 | 2.007 |
| 1 | 121 | 2.027 |
| 0 | 121 | 2.013 |

Racing adds the player, three rivals, dust and the full HUD and settles at
3-4 vblanks per frame.

The band sweep, taken before the trims, gives the column cost directly:

| Bands | Columns | Vblanks |
| ---: | ---: | ---: |
| 0 | 30 | 2.007 |
| 2 | 40 | 2.990 |
| 8 | 130 | 3.010 |
| 10 | 172 | 3.987 |
| 12 | 212 | 4.013 |

Between 40 and 130 columns the cost does not change bucket, which brackets the
per-column cost at **1400 to 2200 cycles**.  A 12 MHz 68000 has about 200,000
cycles per field, so the whole scene may spend roughly **90 sprite columns per
field**, and a full-width road band is twenty of them.

Cartridge telemetry reports `vram_writes = 0` for these frames: the renderer's
cache suppresses every hardware write, so this is not VRAM bandwidth.  It is
the per-column bookkeeping in `ng_renderer_flush`.

## What was already fixed

- **Stale objects.** `native/Makefile` had no header dependencies, so a changed
  struct layout linked old and new objects together and the game read its own
  state at the wrong offsets.  `-MMD -MP` now tracks headers.
- **64-bit fixed point.** `baja_fp_mul` went through libgcc's `__muldi3` plus a
  64-bit shift, about a thousand cycles a call, a few hundred times a frame.
  It is now four 16x16 products.  The hot divisions are gone: the segment
  length is a power of two, the object scale is one 32-bit divide, and the
  speed ratio is a constant multiply.
- **One view per frame.** Object projection rebuilt the road-tangent frame per
  object; it is now built once and shared.
- **FIX layer.** Clearing and comparing all 1120 cells twice a frame cost a
  full field.  Only rows the HUD actually wrote are touched.
- **Backdrop.** Sky and ground were separate full-width layers, forty columns
  between them.  They are one panorama now.

Those took the title screen from six fields per frame to two.

## What is left

The road cannot get much cheaper by drawing less: the near bands are wide
because the road genuinely fills the screen there, and fewer bands means a
visible lateral step at band boundaries during a bend.

The real lever is the cost of submitting a sprite column.  Roughly 1500 cycles
to decide that nothing changed and write nothing is about ten times what a
hand-written Neo Geo sprite loop costs.  A fast path in the Forge68 renderer —
one that skips the per-column recomputation when a command's frame, zoom and
column span are unchanged from the previous frame, or that writes SCB entries
from a prepared list — would return the whole road's cost.  That is a change to
the pinned SDK and is Greg's call, not one to make unilaterally.

The alternative inside BAJANEW is to accept a smaller play area or fewer bands,
both of which cost visible quality.  Neither reaches 60 Hz.
