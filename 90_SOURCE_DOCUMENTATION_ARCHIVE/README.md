# BAJA Outrun

BAJA Outrun is a native 320x224 Neo Geo AES/MVS off-road racer by Max Cruz
Racing. Max (8) and Cruz (6) drive a blue 2022 Polaris RZR Pro R through four
SCORE-inspired Baja locations against independently moving RZR and Maverick
challengers.

The previous software-verified release candidate was rejected by live human
play on 2026-08-30. Its automated driver missed forced menu progression and
idle auto-throttle, and its release verdict is withdrawn while the human play
path and road presentation are rebuilt. Physical AES/MVS hardware remains
unverified.

## Play in MAME

Keep `neogeo.zip` outside this project and point `MAME_BIOS_DIR` at its
directory:

```sh
make rom
MAME_BIOS_DIR=/path/to/neogeo-bios make mame
```

Use the configured Player 1 directions, Button A for throttle/confirm, Button B
for brake, and Player 1 Start. Select Max or Cruz, then race Ensenada, San
Felipe, Valle de Trinidad, and Bahia de los Angeles.

## Build and verify

```sh
python3 tools/convert_baja_art.py
make production-check
make test
make rom
make verify
make docs-check
MAME_BIOS_DIR=/path/to/neogeo-bios make mame-smoke
MAME_BIOS_DIR=/path/to/neogeo-bios make mame-play
```

`make mame-play` drives the actual emulated Neo Geo inputs, records only
consecutive game frames paired with strictly advancing VBlanks, and validates
steering/recentering, throttle/brake/coast, off-road drag and recovery,
collision, distinct rival lane behavior, overtakes and order, all four race
legs, finish flow, road animation, and renderer limits. It writes the durable
trace, JSON summary, log, and 21 native PNGs under `build/evidence/`.

`make mame-evidence` is an alias for this frame-synchronous play gate. `make
rom` writes the `puzzledp` carrier ROMs under `build/roms/`.

## Design and source boundary

The public [ZgzInfinity/OutRun](https://github.com/ZgzInfinity/OutRun) project
is a read-only behavior oracle pinned at
`5d4f5409cc79021eacc757a164ff00515253fc51`. It is GPL-3.0 while this project is
MIT. No upstream implementation, constants, data, maps, art, or trade dress are
copied. The independently implemented behavior contract is documented in
[docs/GAMEPLAY_ORACLE.md](docs/GAMEPLAY_ORACLE.md).

Existing generated-art raws and provenance are verified offline by hash; no
Grok account or process is required. See [docs/README.md](docs/README.md), the
current [Gauntlet scorecard](docs/GAUNTLET_SCORECARD.md), and optional
post-release work in [TODO.md](TODO.md).
