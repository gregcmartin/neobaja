# The requested BAJA game

## North star

Create a real, responsive rear-chase off-road arcade racer for native Neo Geo,
not a sequence of attractive full-screen illustrations. Within seconds of
taking control, the player should feel speed, steering weight, ruts, bumps,
dust, rivals, and the consequences of leaving the road.

## Identity

- Working title: **BAJA Outrun**.
- Developer identity: **Max Cruz Racing**.
- Native target: Neo Geo AES/MVS at 320x224.
- Presentation target: dense, confident late-era Neo Geo pixel craft with the
  polish and visual intensity associated with the platform's best titles.
- Genre: OutRun-style rear-chase arcade racing translated to Baja off-road
  terrain, while keeping all code, data, art, maps, and presentation original.

## Player and vehicles

- Two selectable fictional child racers:
  - Max: 8 years old, blond hair, lighter complexion, jersey number 8.
  - Cruz: 6 years old, brown hair, slightly darker complexion, slightly shorter,
    jersey number 6.
- One player vehicle choice: a large blue 2022 Polaris RZR Pro R.
- Rival field: visually and behaviorally distinct Polaris RZR and Can-Am
  Maverick competitors.
- The player vehicle must read as a machine, with a roll cage, suspension,
  tires, body planes, lights, spare tire, shadow, and terrain-connected poses.

Brand and likeness licensing was not established in the rejected project and
should be treated as an open production decision before public release.

## Core play loop

1. Show the supplied Max Cruz Racing developer splash for exactly five seconds.
2. Wait at a title screen for Player 1 Start.
3. Let the player choose Max or Cruz; do not auto-select.
4. Run a clear countdown.
5. Race through an active pseudo-3D dirt course against independent rivals.
6. Finish the stage, show useful results, and transition deliberately to the
   next location.
7. Complete all four Baja locations as a coherent race journey.

Normal play is entirely player-controlled. No menu may advance and no vehicle
may accelerate because a timer expired. A future attract/demo mode, if wanted,
must be explicitly designed, visibly labeled as a demo, and isolated from the
normal game state.

## Controls and driving feel

- Player 1 Start: leave the title screen.
- Left/Right: progressive steering with readable vehicle lean/pose and smooth
  recentering when released.
- A: confirm in menus and accelerate in the race.
- B: brake in the race.

Throttle should build speed; coasting should gently lose speed; braking should
slow the vehicle more strongly. Short steering taps should make controlled
corrections, while sustained steering should eventually reach the shoulder.
Curves should require correction without dragging the car unpredictably.
Off-road travel must reduce performance and change the visible terrain/dust
response. Collisions must have a readable physical consequence without making
the vehicle uncontrollable.

## Road, camera, and motion

The moving road is the game. It must be projected from course state and share a
coherent coordinate system with the player, rivals, hazards, and scenery.
Full-screen stage art may provide sky or far background layers, but it must not
bake the complete road and gameplay composition into a static plate.

Required motion cues include:

- a strong road funnel from horizon to foreground;
- continuously approaching ruts, grooves, stones, berms, and terrain marks;
- curves and elevation that change the projected racing line;
- multiple parallax depth layers and foreground occlusion;
- rivals that scale and move laterally in the same world;
- dust emitted from tires, grounded shadows, suspension compression/rebound,
  steering poses, bumps, jumps, and contact feedback;
- stage dressing that enters and leaves view instead of remaining painted in
  one fixed screen position.

At native 1x, the road, immediate hazards, rivals, and player response must be
readable even while the HUD is visible.

## Rival behavior

The RZR and Maverick are participants, not decorative sprites or telemetry
values. Each must independently approach, select a bounded lane, react to
proximity, be passed, leave the forward field, and affect position feedback.
At least two behavior personalities should be obvious during ordinary play,
such as a blocking rival and a more evasive rival. A collision requires both a
visual contact event and a gameplay consequence.

## Four stage identities

| Location | Visual identity | Course/event identity |
| --- | --- | --- |
| Ensenada | Bright Pacific daylight, turquoise water, cliffs, palms, crowd and helicopter energy | Coastal road, cliff-side bends, fast opening leg |
| San Felipe | Hot open desert, broad sky, cactus, scrub, spectators and long visibility | Rutted straight, rival pack pressure, high-speed desert racing |
| Valle de Trinidad | Orange canyon sunset, layered mesas, long shadows and chevrons | Elevation change, crest and readable jump, technical canyon route |
| Bahia de los Angeles | Moonlit cobalt storm, lightning, cacti, wet/cool shadows and warm checkpoint lights | Night visibility challenge, storm atmosphere and final checkpoint drama |

The stages must differ in geometry, depth layers, palette/value structure,
weather or lighting, dressing, and race events. A palette swap over one shared
static scene does not satisfy the request.

## HUD and presentation

- Upper left: position and stage/leg progress.
- Upper center: prominent race time.
- Upper right: best and lap/stage timing.
- Lower left: speed, gear, and tachometer.
- Lower right: full stage name and readable minimap/route progress.

Recompose this hierarchy for 320x224. Type must be hand-controlled bitmap text,
not malformed generated lettering. The play lane remains clear, stage names do
not wrap into the vehicle or minimap, and changing values must be useful during
play.

## Art direction

- The four user mockups define composition, vehicle presence, depth, stage
  identity, lighting, density, and HUD ambition.
- BAJA: Edge of Control HD supplies secondary observations about terrain,
  suspension, dust, race packs, and physicality.
- References are never source pixels. Do not trace, repaint, ship, or imitate
  logos, liveries, sponsor marks, camera framing, or trade dress.
- Author new elements for native pixel use: deliberate clusters, selective
  outlines, stage-specific palette ramps, hard transparency, controlled
  dithering, and coherent animation.
- Build modular sprite/terrain/effect layers. Do not downsample one large AI
  illustration into a static gameplay screen and call it a road engine.

## Definition of success

The game succeeds only when Greg can launch it in MAME, wait at each menu,
choose a racer, drive deliberately for at least five minutes, understand the
road and rivals, recover from mistakes, and say that the core loop feels worth
continuing. Technical proof and beautiful stills come after that human gate.

