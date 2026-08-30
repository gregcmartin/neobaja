using our SDK ~/neogeo develop a BAJA Racing outrun style game for Neogeo themed as baja off road racing.  The game developer is Max Cruz Racing. There are two playable characters Max and Cruz both kids Max is 8 and Cruz is 6 years old Max has blond hair and white complextion Cruz is brown hair and slightly darker complextion and slightly shorter than Max.

There only truck option is a 2022 polaris rzr pro r

The AI competitors will be other polaris rzr and can-am mavericks

Use this game as a basics of the game engine and mechanics: https://github.com/ZgzInfinity/OutRun

use real locations and tracks from the actual score races as levels make sure the backgrounds are detailed and realalistic

Make this a AAA neogeo game pushing the limits of the hardware maximizing high end pixel, rasters and art competing with top titles like metal slug 3 in terms of overall quality. 

The bar to meet for top tier Baja Racers is Baja: Edge of Control
use that as a reference for quality but remember we are making the NeoGeo version

## Build

```sh
python3 tools/convert_baja_art.py
make production-check
make test
make rom
make verify
MAME_BIOS_DIR=/path/to/neogeo-bios make mame-smoke
MAME_BIOS_DIR=/path/to/neogeo-bios make mame-evidence
```

`make rom` writes the puzzledp carrier under `build/roms/` (`202-p1.p1`,
`202-s1.s1`, `202-m1.m1`, `202-v1.v1`, `202-c1.c1`, `202-c2.c2`,
`puzzledp.zip`). BIOS files stay outside this tree. See [docs/README.md](docs/README.md)
and leftover AAA scope in [TODO.md](TODO.md). The clean-room behavioral gates
derived from the linked OutRun project are documented in
[docs/GAMEPLAY_ORACLE.md](docs/GAMEPLAY_ORACLE.md).

Build and provenance verification are offline: the existing generated-art
records, prompts, raw-output hashes, and approvals are checked locally and do
not require a running or authenticated Grok CLI. The Codex continuation handoff
is in [AI/README.md](AI/README.md).
