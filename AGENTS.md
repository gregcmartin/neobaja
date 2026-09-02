# BAJANEW agent instructions

Start with `START_HERE.md`, then read the curated documents in
`01_GAME_VISION/` and `02_REFERENCE_LIBRARY/README.md` before proposing or
building anything.

This is a semi-clean-room restart packet. Do not import or adapt source code,
engine code, constants, tools, tests, converted assets, generated art, ROMs,
telemetry scripts, or build configuration from `/Users/gregmartin/Desktop/BAJA
Outrun`. The prior project may be inspected only through the quarantined
documentation and screenshots already copied here, unless Greg explicitly
widens that boundary.

The four files under `02_REFERENCE_LIBRARY/primary-user-mockups/` are the
primary visual target and are inspection-only. Never ship, trace, crop,
repaint, or use their pixels as an implementation asset. The eight BAJA: Edge
of Control HD screenshots are secondary copyrighted references and are also
inspection-only.

Greg rejected the 2026-08-30 BAJANEW game implementation after live play. Do
not recover, inspect, import, adapt, or benchmark its source, engine behavior,
constants, architecture, tests, build configuration, tools, ROMs, converted
assets, emulator state, runtime evidence, or automated scores from Git tag
`rejected-bajanew-playability-v1` or commit `c24d1fe`. The original artwork
under `art/` is the sole implementation-era exception and is approved for reuse.

The pinned GPL-3.0 OutRun repository is a behavior-observation oracle only.
Carry forward relational behavior such as throttle versus brake/coast,
steering and recentering, road consequences, rival independence, collision,
and race progression. Do not copy or port source, constants, data, maps,
assets, presentation, names, or trade dress.

Do not revive UltraGrok. On 2026-09-02 Greg widened the art boundary: new
bitmap art may be generated with his Grok Build account when the approved raws
do not cover a need. Retain the unaltered raw under `art/raw/grok/`, identify
the real provider/model, and record honest hashes and conversion steps in
`art/ART_PROVENANCE.md` and `art/PROMPTS.md`.

Human play is a release gate. Automated tests, telemetry, screenshots, and
scorecards can support a verdict but cannot override Greg's live MAME result.
Menus must wait for real input, gameplay must never inject idle throttle, and
any future attract mode must be visibly labeled and isolated from normal play.

Build one stage as a vertical slice and obtain human approval before producing
the remaining stages or a full art inventory. Preserve source files and create
a rollback before any broad change.
