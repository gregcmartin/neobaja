# Gameplay preservation oracle

BAJA Outrun uses the public
[ZgzInfinity/OutRun](https://github.com/ZgzInfinity/OutRun) project named in
the root README as a gameplay-behavior reference. The inspected upstream is
commit `5d4f5409cc79021eacc757a164ff00515253fc51`.

The upstream project is GPL-3.0 and BAJA Outrun is MIT. No upstream source,
constants, assets, data, maps, names, or presentation are copied into this
project. Its source is used only to identify racing relationships that BAJA
implements independently in fixed-point C for the Neo Geo.

The reference areas are:

- `PlayerCar.cpp`: throttle, coast, braking, steering/recentering, road-surface
  consequences, curve force, and collision response.
- `TrafficCar.cpp`: multiple proximity-aware opponent behaviors.
- `Map.cpp`: opponent activation, passing, forward projection, curves, road
  limits, checkpoints, and course progression.
- `Game.cpp`: input gating, timers, round lifecycle, and finish flow.
- `HudRound.cpp`: live speed, gear, timer, checkpoint, and route feedback.

## Regression contract

`make test` includes clean-room relational tests for:

- monotonic throttle to a capped speed;
- braking stronger than equal-duration coasting and no negative speed;
- opposite left/right response and steering recentering;
- speed-driven course/camera progression and perspective;
- independent, visually distinct RZR and Maverick challenger movement;
- a challenger approaching, leaving the forward field after an overtake, and
  changing place feedback;
- collision consequences and non-collision stability;
- deterministic replay of the same fixed input sequence;
- legal transitions through all four named stages and wrap to Ensenada;
- Max/Cruz selection, controller mapping, and the player/rival identities.

The final Gauntlet also requires a real off-road performance penalty and live
MAME telemetry for steering, rival approach/lane change, pass/order change,
collision/non-collision, finish, and next-stage. `make mame-play` enforces this
contract against the real emulated controls and all four 1,000-plus-frame race
legs. Visual repairs that do not need gameplay changes should leave simulation
and input code untouched.

## Evidence rule

Stage A/B captures must be separated by unique gameplay VBlanks and meaningful
telemetry changes. MAME frame-notifier callback counts alone are not evidence
of elapsed gameplay. An overall Gauntlet PASS requires both the frozen visual
scorecard and this gameplay oracle to pass.
