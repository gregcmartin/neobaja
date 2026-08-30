# BAJA Outrun agent instructions

## Codex continuation state

The UltraGrok workflow was intentionally stopped for a Codex handoff on
2026-08-30 at 13:18 CDT. It is archived as `interrupted` and cannot resume.
There should be no live Grok or MAME writer. Start by reading, in order:

1. `AI/README.md`
2. `AI/RESUME_PROMPT.md`
3. `AI/CURRENT_STATE.md`
4. `AI/CODEX_HANDOFF.md`
5. `docs/GAUNTLET_SCORECARD.md`

Do not launch UltraGrok or require a live Grok account. Existing Grok-generated
raws and provenance remain valid historical inputs and are verified offline by
hash. Codex may repair code, conversion, native pixel craft, tests, and MAME
evidence directly. If new bitmap generation is genuinely needed, use Codex's
image-generation capability, retain the raw result, and extend provenance
honestly for its actual provider before shipping; never relabel new art as
Grok-generated.

The target is not a Git repository. Preserve user files and make an explicit
rollback copy before broad or risky changes.

Before changing gameplay, input, collision, course progression, renderer/game
coupling, or target timing, read `docs/GAMEPLAY_ORACLE.md` and run the BAJA host
tests before and after the change.

The OutRun repository linked by the root README is a read-only gameplay
behavior oracle pinned at commit
`5d4f5409cc79021eacc757a164ff00515253fc51`. It is GPL-3.0 while this project is
MIT. Do not copy or port its source, constants, assets, data, maps,
presentation, or trade dress. Preserve only independently implemented
behavioral relationships.

Visual repair should not alter `game/baja_sim.c`, `include/game/baja_sim.h`, or
input/collision/state code unless the repair genuinely requires it. Any such
change must preserve throttle, brake and coast relationships; steering and
recentering; off-road consequences; deterministic fixed-step progression;
collisions; the four-stage race lifecycle; Max/Cruz selection; and distinct,
independently moving RZR and Maverick challengers that can approach, change
lane, be overtaken, and affect order feedback.

An overall Gauntlet PASS requires both the frozen visual scorecard and gameplay
oracle. Use sequential live MAME telemetry and frames separated by unique game
VBlanks; notifier callback counts or near-identical zero-speed A/B captures are
not motion evidence.
