# Programming reference register

These projects are inspection-only references for the clean-room BAJANEW
implementation. They are not vendored, linked, compiled, converted, or packed
into the cartridge. No source constants, course data, ROM data, names, art,
audio, or trade dress are copied into BAJANEW.

## Pinned references

### Cannonball

- Repository: `https://github.com/djyt/cannonball`
- Inspected commit: `27493ebf62be3498dff93ed6a45e8e2db819bae1`
- Role: behavior observation for the relationships among speed, steering,
  road curvature, traffic depth, lane decisions, collision, and overtaking.
- Integration status: reference only. Cannonball is a C++/SDL/Boost engine for
  modern computers and requires original OutRun ROM data. It is not a Neo Geo
  engine and no Cannonball source or data ships here.

### OutRun Arcade Software Development Kit

- Article:
  `https://reassembler.blogspot.com/2016/07/out-run-arcade-software-development-kit.html`
- Role: hardware-architecture reference showing how an arcade racer can split
  input, palette, tilemap, sprite, text, menu, and boot responsibilities.
- Integration status: reference only. The SDK targets original Sega OutRun
  hardware, not Neo Geo hardware. No SDK library, example, binary, bootloader,
  constant, or asset ships here.

### Pseudo3D-road

- Repository: `https://github.com/BojanSof/Pseudo3D-road`
- Inspected commit: `62623de658fee3a96e14b7b4b99cddb12c1bd633`
- Role: simple perspective sanity reference: translate world coordinates into
  camera space, divide by positive depth, scale to screen, interpolate curves,
  and clip road spans hidden by hills.
- Integration status: reference only. It is a C++14/SFML desktop demonstration,
  not a gameplay engine and not a Neo Geo renderer. No source or assets ship
  here.

## Independent implementation rule

BAJANEW owns a separate coordinate model, fixed-point representation, course
data, tuning values, input state machine, physics, AI, collision response,
projection API, renderer, tests, and build. Reference inspection may establish
relationships and failure cases, but never supplies an implementation value.

The approved original Baja art under `art/` remains the only artwork eligible
for the Ensenada vertical slice.
