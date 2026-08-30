# Sources

Code, tools, tests, and visual assets in this worktree were authored here.
The Forge68 SDK was copied as a read-only foundation and adapted in this
project. No sibling BAJA tree was used as a source. The pinned GPL-3.0 OutRun
checkout was inspected read-only for behavioral relationships in player input,
traffic, course progression, lifecycle, and HUD feedback. BAJA implements those
relationships independently in fixed-point C; no OutRun implementation,
constants, assets, data, maps, presentation, or trade dress were imported.

## Grok generation

- Tool: grok-build 1.0.13
- Model: grok-4.6 / grok-imagine image tools
- Session: `01a05371-ecfe-75c3-9686-b374741dc1da`
- Raw rasters: `art/raw/*.jpg` with SHA-256 recorded in `art/provenance/`
- Conversion: `tools/convert_baja_art.py` nearest-neighbor, hard alpha,
  Neo Geo 5-bit posterize, then `tools/ngasset.py` 15-color banks

Splash exception: `devsplashlogo.jpg` SHA-256
`6e01d4f3fdb9daaa6feb90b52ab0497e3f26b5191c6ac205b0efeed1ed6eeba1`, converted
to `assets/source/splash.png` at build time. FIX font is bitmap typography,
not an illustrative raster.

Hardware references:

- NeoGeoDev sprite, palette, FIX, and 68k header documentation
- MAME Neo Geo driver and Lua scripting docs
- ZgzInfinity/OutRun commit `5d4f5409cc79021eacc757a164ff00515253fc51`
  as the behavior-only oracle described in `GAMEPLAY_ORACLE.md`
- SCORE International public race geography for Ensenada, San Felipe,
  Valle de Trinidad, and Bahia de los Angeles

User mockups and BAJA: Edge of Control HD screenshots in
`art/workbench/reference/` are inspection-only and never compiled.
