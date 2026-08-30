# BAJA Outrun — Ensenada vertical slice

This is a new native Neo Geo AES/MVS implementation built with the installed
LGPL `ngdevkit` toolchain. It does not import source, constants, build files,
tests, tools, ROMs, telemetry, converted art, or generated art from the rejected
`BAJA Outrun` project.

The current deliverable is intentionally one stage. It includes the exact
five-second Max Cruz Racing splash, title and racer selection that wait for
real input, countdown from rest, a projected moving Ensenada dirt road, a
controllable blue side-by-side, one independent orange rival, off-road and
collision consequences, HUD, finish, and restart.

## Build and verify

```sh
make test
make rom
make verify
make mame-smoke MAME_BIOS_DIR=/path/to/neogeo-bios
make mame-play MAME_BIOS_DIR=/path/to/neogeo-bios
```

The cartridge is written to `build/rom/bajanew.zip`. BIOS files stay outside
this repository. `mame-play` opens the AES build at a 3x integer scale for the
required unscripted session.

## Controls

- Player 1 Start: title to selection
- Left/Right: choose racer or steer
- A: confirm / throttle / restart
- B: brake

Normal play never advances a menu or applies throttle without input. A future
attract mode is not implemented.

## Required human gate

Automated results do not approve the game. Launch it in MAME, play with normal
controls for at least five continuous minutes, and evaluate every item in
`01_GAME_VISION/PLAYABILITY_AND_ACCEPTANCE.md`. Expansion beyond Ensenada is
blocked until Greg approves that live session.
