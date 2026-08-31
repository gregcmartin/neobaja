# Programming-slice verification

Verified on 2026-08-30 with Forge68 `f52c1a0`, MAME, and a user-supplied Neo Geo
BIOS. Generated evidence stays under ignored `build/native/evidence/`.

## Passing automated gates

- Ten deterministic fixed-point simulation/projection tests.
- Address/undefined-behavior sanitizer run.
- Standalone 68000 compilation of the simulation core.
- Deterministic full-race trace with independent rivals, collision, overtake,
  and finish progression.
- Native host run with no dropped columns or overloaded scanlines.
- Cartridge ROM verification.
- MAME boot with advancing telemetry, non-empty 320x224 screenshot, and audio.
- MAME controller profile resolution: arrows, A throttle, B brake, and 1 Start.
- MAME input behavior: idle stays stopped; A accelerates; coast reduces speed;
  B reduces speed more strongly than coast; left and right move laterally in the
  requested directions; Cruz #17 selection reaches gameplay.

## Deliberately not passed yet

- The current wireframe/FIX presentation is a programming instrument, not the
  approved Ensenada visual slice.
- The Forge68 MCP server is pinned and its protocol is understood, but its
  current game serializer targets the SDK demo, not BAJANEW.
- Greg's unscripted five-minute MAME play verdict remains the release authority.
- No remaining stages or broad new art inventory are authorized before that
  human vertical-slice approval.
