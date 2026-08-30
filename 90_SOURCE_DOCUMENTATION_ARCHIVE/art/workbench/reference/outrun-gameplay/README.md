# OutRun gameplay behavior oracle

This nonshipping reference protects BAJA Outrun's racing game while visual
agents replace and polish its art. It is derived by inspecting the exact
repository named in the root README, not from another local BAJA project.

## Pinned upstream

- Repository: `https://github.com/ZgzInfinity/OutRun`
- Inspected commit: `5d4f5409cc79021eacc757a164ff00515253fc51`
- Commit date: `2023-06-08T00:35:33+02:00`
- Upstream license: GNU GPL v3
- BAJA Outrun license: MIT

The upstream is an observation oracle only. Do not copy or translate its
source, constants, data, assets, names, maps, presentation, or trade dress
into BAJA Outrun. Record behavioral relationships in original tests and use
BAJA's existing fixed-point Neo Geo implementation.

## Source areas to inspect

Inspect these files at the pinned commit before changing gameplay code:

- `src/Car/PlayerCar/PlayerCar.cpp`: throttle, braking, coast drag, steering
  buildup/recentering, surface limits, centrifugal curve force, and collision
  consequences.
- `src/Car/TrafficCar/TrafficCar.cpp`: distinct passive, evasive, and blocking
  traffic behaviors and proximity-triggered lateral decisions.
- `src/Scene/Map/Map.cpp`: traffic activation/recycling, overtaking score,
  forward course movement, curves, road boundaries, collisions, checkpoints,
  forks, and stage progression.
- `src/Game/Game.cpp`: round lifecycle, difficulty/traffic setup, input gating,
  timer, finish conditions, and transition flow.
- `src/Gui/Huds/HudRound/HudRound.cpp`: live speed, gear, timer, checkpoint,
  and route progress feedback.

## Required BAJA behavior relationships

These are clean-room relational invariants, not upstream numeric constants:

1. Holding throttle from rest increases speed monotonically until a cap.
2. Braking reduces speed more per equal interval than coasting; neither path
   may underflow below zero.
3. Left and right input produce opposite steering and lateral movement.
   Released steering converges toward center instead of sticking.
4. Forward speed changes distance, camera/course projection, road segment
   positions, and stage progress. Curves influence the projected road and
   require steering correction.
5. Leaving the driveable road must have a real consequence: lower attainable
   speed, extra drag, reduced traction, or another documented performance
   penalty, plus visible terrain/dust feedback.
6. The RZR and Maverick challengers remain distinct entities. They advance
   independently, retain different identity/parameters, make bounded lateral
   decisions, enter and leave the player's view, can be overtaken, and affect
   position/order feedback.
7. At least two challenger behavior profiles must be observably different;
   one cannot be a second sprite attached to the same motion path.
8. Overlap alone is not enough: a collision must set collision state and cause
   a measurable speed and/or lateral consequence. A non-overlap must not.
9. Countdown, running, finish, next-stage, and the four named stage transitions
   remain legal and ordered. Completing the fourth stage wraps or ends only as
   documented; visual code cannot short-circuit the race.
10. Given the same initial state and fixed input sequence, two runs end with
    identical gameplay state. Rendering, art conversion, and capture code must
    not mutate simulation outcomes.
11. Max/Cruz selection and controller bindings remain functional, while the
    player vehicle remains a blue 2022 Polaris RZR Pro R and rivals remain
    visually and logically distinct RZR/Can-Am Maverick challengers.

## Mandatory regression evidence

Before any visual repair that touches `game/baja_sim.c`,
`include/game/baja_sim.h`, input, collision, scene progression, renderer/game
coupling, or target timing:

- run the host gameplay tests and record the result;
- run the same tests after the repair and compare relational outcomes;
- run a deterministic twin-input trace;
- rebuild the ROM and run MAME smoke;
- capture a sequential live-race telemetry trace proving changing speed,
  distance/camera, steering/lateral state, rival relative distance, collision,
  pass/order, finish, and next-stage state;
- capture stage A/B frames only after enough distinct game VBlanks or telemetry
  delta to make motion visible. MAME frame notifier callbacks are not a valid
  substitute for unique game frames.

No visual score can compensate for a failed behavior invariant. If a visual
repair does not require gameplay changes, keep gameplay files untouched.

## Gauntlet critic protocol

Every critic must report both verdicts:

- `VISUAL`: the frozen seven-category image scorecard.
- `GAMEPLAY`: PASS only when all behavior relationships and live telemetry
  gates pass.

The overall result is PASS only when both verdicts pass. A gameplay failure
spawns a fresh gameplay repair specialist; a visual failure spawns a fresh
visual repair specialist. The next independent panel must reassess both so a
repair in one dimension cannot regress the other.
