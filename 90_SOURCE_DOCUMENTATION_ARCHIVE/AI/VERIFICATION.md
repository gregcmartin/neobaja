# Verification after Codex completion

Validated on **2026-08-30** with the archived Grok workflow stopped. No live
Grok account, process, or MAME writer is required for offline rebuilding.

## Clean reproducibility

`python3 tools/convert_baja_art.py` rebuilt all 30 native sheets. Two consecutive
`make clean && make check` runs passed and produced byte-identical ROM hashes:

- `202-p1.p1` — `f58725dfe9077bc8242ea9b5650ff9a3b9f23412fe1c25db6e7de20c987fd0d5`
- `202-s1.s1` — `2ee333826090794fb0d308b53e79e384e239e5d42785ecaf539a81ec08e49090`
- `202-m1.m1` — `7a788e52247104bf3d4d677964007d843b64ac4f3f7d929a882d9542b567fe72`
- `202-v1.v1` — `07854d2fef297a06ba81685e660c332de36d5d18d546927d30daad6d7fda1541`
- `202-c1.c1` — `56791a41b8e388b87c8ae6cd4f45f78c46b38d0bcaffb3c0e5e50c45d1894525`
- `202-c2.c2` — `9d1e6172c84d580e98e7a35a9ac63b180bdcc6be9df97853d10c0f2a707b9664`
- `puzzledp.zip` — `9bbd283e4fd03be9e99f792b3f6e6f8950760f2d19702956adb50629474dea6a`

`make check` results:

- production check and 30/30 development art audit: PASS;
- engine and BAJA C host tests: PASS;
- 12 Python tests: PASS;
- 1,200-frame host run: no dropped columns or overloaded scanlines;
- ROM verify: 1,620 unique tiles, 25.6% reference dedup, 126,090-byte ZIP;
- ELF: text 29,094, data 0, BSS 8,198, total 37,292 bytes;
- docs check: 42 Markdown files and 8 local links;
- report: PASS, 30/30 production art.

## Native MAME

The post-clean smoke test passed at native 320x224:

`MAME_SMOKE_PASS frames=520 columns=74 peak=54 overloaded=0`

The final input-driven playthrough passed with:

- 6,149 telemetry records, game frames 0–6,148 consecutive;
- VBlanks 0–23,953 strictly increasing;
- 21 native visual checkpoints;
- race-leg frames: Ensenada 1,292; San Felipe 1,299; Valle 1,268; Bahia 1,329;
- throttle 5,305, left 56, right 56, brake 12 input samples;
- on-road maximum 26, off-road maximum 13, recovery observed;
- 8 collision and 5,180 non-collision running frames;
- both rivals approached, changed lanes, were passed, and yielded places 3/2/1;
- four foreground road frames plus 86/341/129/88 changed crop pixels by stage;
- road-center near-black ratios 6.62%, 5.39%, 5.58%, and 8.09%, all below the
  12% void threshold;
- 83 maximum active columns, 59/96 peak scanline columns, zero drops and zero
  overloads.

Authority: `build/evidence/mame-play-summary.json`, the full JSONL trace, MAME
log, and 21 `baja-play-*.png` files. Every PNG cited by the final scorecard was
opened directly after the final gameplay changes. Physical AES/MVS remains
unverified.
