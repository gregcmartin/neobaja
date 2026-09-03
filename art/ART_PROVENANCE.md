# Vertical-slice art provenance

## Clean-room reset status

Greg approved this artwork while rejecting the game implementation and
playability. The raw images, prompts, and hashes below are intentionally
preserved. The converter, generated cartridge assets, palette/tile layout,
runtime dimensions, build tooling, and ROM were part of the rejected
implementation and were deleted. A future rebuild must create a new conversion
and integration pipeline rather than recovering the old one.

All shipping illustrative raster sources in this vertical slice are new outputs
from the built-in OpenAI `image_gen` provider. The tool did not expose a model
identifier, so none is invented here. No reference image was supplied to the
generator or used as an edit base. The unchanged raw outputs are under
`art/raw/openai/01a05452-0194-7532-93b8-bca5bd770d7c/`.

The prompt specifications are recorded in `art/PROMPTS.md`; the final
environment prompt is retained verbatim. Raw and converted hashes,
tile offsets, and the deterministic conversion description are emitted to
`build/assets/manifest.json` by `tools/prepare_assets.py`.

The second Ensenada environment pass is retained unchanged as
`ensenada-full-environment.png` (SHA-256
`a5597b831fba7e3d1bf5c9e201d6bc956ebf6b048b3d8b36be2f8a6ccc095fdc`).
It was generated from text only. No mockup or copyrighted screenshot was
attached to the generation request.

The revised racer-selection source is retained as `max-cruz-select-v2.png`
(SHA-256
`1669c99ad3bc25e45002d4e1f07358ae061da9074257630871b17184e04dc891`).
It is a built-in `image_gen` edit of the original project-owned portrait:
Max's outfit changed to red and number 2, while Cruz's changed to blue and
number 17. The original raw remains alongside it unchanged.

The Max Cruz Racing developer splash is the sole user-provided shipping image.
Its pinned source remains unchanged at
`02_REFERENCE_LIBRARY/developer-splash/devsplashlogo.jpg` and is converted with
aspect ratio preserved into a 320x224 matte presentation.

The bitmap font source is public-domain Unscii from the official ngdevkit
example package. Its local notice is in `third_party/unscii/README.md`.


## Grok Build additions (2026-09-02)

Greg widened the art boundary on 2026-09-02 to allow his Grok Build account.
Seventeen raws under `art/raw/grok/` were generated headlessly (grok-4.6, Imagine
`image_gen`) from the text prompts in `art/PROMPTS.md`; no reference image,
mockup or screenshot was supplied to the generator.  They are kept exactly as
the tool saved them and hashed in `art/raw/grok/SHA256SUMS`.

Conversion (`tools/grok_props.py`, `tools/build_assets.py`): the generator's
painted checkerboard or sky background is recovered by flood filling from the
image edge through background-coloured pixels, the cut-out is area-averaged
into its frame and quantised to a fifteen colour palette.  Four raws whose
painted sky could not be separated from the subject were repainted by Grok
onto a flat magenta field (`*_keyed.jpg`, prompt in `art/PROMPTS.md`) and
keyed from that.  The billboard face
and the arch banner are lettered with the game's own bitmap typeface.  The
rival buggy is cut once and shrunk to four levels of detail.

The road strips are rectified from the Grok Build road plate rather than the
environment plate since 2026-09-02; the environment plate remains the backdrop
source.  The tachometer face is converted into FIX tiles on the fifteen colour
HUD palette; its needle is drawn by `tools/build_assets.py` as a rotating
pointer, a UI element rather than illustration.

Two more Grok Build raws on 2026-09-03, generated the same headless way from
the prompts in `art/PROMPTS.md` with no reference image: the sun over the
coast (`sun.jpg`, a 32 by 32 sprite with its own palette so the sky keeps its
colours) and the wide dust bank (`dust_wide.jpg`, three sizes as the dust
behind the player and the near rivals).

The tachometer face is drawn by `tools/build_assets.py` since 2026-09-03 (a
dial with rim, ticks, red line and numerals from the game's typeface, the
needle sprite carrying the dark centre); the Grok Build gauge face is no
longer converted.  The road surface takes a warm colour grade in
`tools/road_strips.py` (`WARM_GAIN`, `WARM_GAMMA`) after rectification.
