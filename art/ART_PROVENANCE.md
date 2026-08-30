# Vertical-slice art provenance

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
