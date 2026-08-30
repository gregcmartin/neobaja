# What Greg wanted versus what we built

The project failed at the level that matters: normal live play. The old process
over-certified implementation state and attractive still images while Greg's
unscripted MAME session found a game that was not enjoyable or convincingly
drivable.

## Comparison

| Greg's requested experience | Rejected implementation | Restart rule |
| --- | --- | --- |
| A player-controlled arcade racer | Menus previously advanced after a short idle period and gameplay injected idle throttle; the automated driver pressed inputs immediately and missed both defects | Menus wait indefinitely, rest means zero input, and an ordinary human session is the first gate |
| A moving pseudo-3D dirt road | Most of each stage was a fixed full-screen illustration with perspective and road already baked in; only small foreground overlays changed | The projected road and course state must drive the whole playable depth field |
| Speed that is obvious by looking | Telemetry changed faster than the visual scene; A/B stills were used to argue motion even when the player experience remained static | Low, medium, and high speed must be unmistakable in continuous play without telemetry |
| Coherent chase-camera depth | Player, rivals, baked road, and overlay did not share one convincing perspective; rivals could appear to float above painted terrain | Road, rivals, player, hazards, shadows, and scenery use one coordinate/projection model |
| Responsive, learnable handling | Steering was initially so aggressive that small input could cross the road quickly; collision displacement compounded the problem | Tune with a human controller: small taps correct, sustained input moves, release recenters, mistakes remain recoverable |
| Real off-road physicality | Off-road state could be proven numerically, but terrain contact, tire response, dust, and recovery did not carry enough visual weight | State and pixels agree: surface, speed, traction, pose, dust, and sound all change together |
| Rival RZR and Maverick competitors | Rival state existed and lanes changed, but the sprites were often visually detached from the terrain and did not create a convincing race pack | Rivals visibly approach, ground, react, collide, get passed, and affect position during ordinary play |
| Four distinct moving Baja courses | Four attractive stage plates changed location and palette, but geometry and scenery were largely static compositions | Each stage changes projected geometry, elevation, dressing flow, lighting/weather, and a gameplay event |
| Late-era Neo Geo pixel craft | Large generated illustrations were reduced into static indexed screens; dense source detail became noisy clusters, while modular animation remained thin | Author modular native-scale layers, sprites, road pieces, effects, and animation with controlled palettes |
| Clear 320x224 HUD | The data was present, but long stage names and lower-corner elements competed with the road and vehicle | Recompose and test every state at native 1x; information cannot cover the driving line |
| Honest release confidence | A 24/28 visual score and scripted telemetry PASS were treated as readiness despite no convincing human play gate | Greg's live result has veto power; automation supports but never overrules it |

## Root design mistake

The build treated a gameplay screenshot as the main art asset. That can produce
a handsome still, but a rear-chase racer needs the road, terrain, rivals,
effects, and camera to be assembled from moving layers. Because most depth was
baked into one image, subsequent road strips and telemetry could not create a
coherent sense of driving through the world.

## Process mistake

The team tried to finish four stages, provenance, scorecards, and a full
automated race before asking whether one stage was fun under an ordinary
controller. Scripted input hid idle behavior, test success was confused with
playability, and static-frame scoring was too generous.

After the first rejection, Codex removed forced menu progression and idle
throttle and softened steering in the old source. Those changes do not rescue
the rejected architecture, and no old implementation file is approved for the
restart.

## What may carry forward

- Greg's explicit concept, characters, vehicle choice, rivals, locations, and
  visual priorities.
- The four primary mockups as inspection-only visual direction.
- Secondary reference observations about physicality and terrain.
- Clean-room relational gameplay expectations.
- The exact developer splash.
- Negative lessons in this document.

## What must not carry forward

- Old source, engine adaptation, constants, data structures, build system, ROM
  layout, tests, telemetry driver, or scripted MAME player.
- Old generated art raws, converted assets, full-screen gameplay plates,
  prompts, or provenance as production inputs.
- Any old PASS label, numerical score, or claim that a gate proves fun.
- Reference pixels, proprietary branding, or GPL implementation details.

## Better restart sequence

1. Agree on the one-stage vision and open design decisions.
2. Prototype the road/camera/handling using obvious diagnostic shapes.
3. Let Greg play that prototype in MAME before producing final art.
4. Add one grounded rival and surface/collision feedback; play again.
5. Replace diagnostic graphics with modular native art and effects; play again.
6. Freeze the approved vertical slice, then scale its proven systems to the
   remaining three stages.

