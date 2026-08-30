# BAJA Outrun development bible

BAJA Outrun is a native Neo Geo AES/MVS cartridge built on a copied Forge68
clean-room runtime. The playable loop lives in `game/baja_sim.c` and
`game/baja.c`. Engine code under `engine/` and `target/neogeo/` is the adapted
Forge68 foundation.

## Hardware

- 320x224 AES/MVS, puzzledp carrier
- P1 512 KiB, S1 128 KiB, M1 128 KiB, V1 512 KiB, C1/C2 1 MiB each
- Sprite scanline budget 96 columns, 381 hardware sprites

## Gameplay

- Developer splash for VBlank 0-299 inclusive, unskippable, then title
- Title identifies Max Cruz Racing and BAJA Outrun
- Select: Max (8, blond, jersey 8) and Cruz (6, brown hair, shorter, jersey 6)
- Player vehicle is the blue 2022 Polaris RZR Pro R
- AI rivals are a Polaris RZR and a Can-Am Maverick
- The RZR uses a blocking lane profile; the Maverick uses an evasive profile
- Controls: LEFT/RIGHT steer, A accelerate, B brake
- Steering recenters, course curves exert speed-dependent lateral force, and
  crossing the road edge applies extra drag plus a lower speed cap
- Pseudo-3D dirt course: `scale = focal / (z - camZ)`
- Stages: Ensenada, San Felipe, Valle de Trinidad, Bahia de los Angeles
- Race HUD: POS/LAP upper left, TIME center, BEST/LAP TM right, speed/gear
  lower left, stage name plus minimap lower right. POS reports the three live
  racers and LAP reports the four sequential Baja legs.

## Live verification

`make mame-play` drives MAME's real emulated Neo Geo controls and validates the
complete race from consecutive cartridge frames and strictly advancing
VBlanks. Its 64-byte telemetry contract includes gameplay state, stage, speed,
camera/distance, steering/lateral position, both rival relative/lateral values,
collision, off-road state, place, input, profiles, player pose, and road frame.
The runner also rejects road voids, static A/B captures, and renderer overload.

## Art

Illustrative sheets are generated with Grok Build `image_gen`/`image_edit`,
hashed under `art/raw/`, then converted with nearest-neighbor, hard alpha, and
5-bit posterize by `tools/convert_baja_art.py`. FIX type is deterministic
outlined bitmap typography. The splash is the pinned user JPEG converted at
build time. Physical AES/MVS soak remains UNVERIFIED.
