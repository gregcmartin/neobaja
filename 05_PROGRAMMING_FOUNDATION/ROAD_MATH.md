# BAJANEW 2.5D road and gameplay math

This is the programming contract for the clean-room vertical slice. The game
state lives in world/track coordinates. Screen coordinates are a disposable
rendering result and never feed back into physics, AI, collision, or race
position.

## 1. Fixed simulation clock

Gameplay advances in one deterministic step per game VBlank. On NTSC Neo Geo
the design target is 60 steps per second. No mechanic reads wall-clock time and
no renderer call mutates gameplay state.

All gameplay values use signed 16.16 fixed point:

```text
one world unit = 65536
multiply(a,b)  = (a*b) >> 16
divide(a,b)    = (a << 16) / b
```

The host and 68000 builds must produce identical state for the same initial
state and input sequence.

## 2. Coordinate spaces

Each moving vehicle has:

- `s`: distance along the course;
- `e`: lateral offset from the road centerline;
- `v`: forward speed;
- `u`: steering state.

The road is sampled as centerline points `(x(s), y(s), s)`. Curvature changes
heading, heading changes centerline position, and elevation changes road height.
This double integration is what makes a bend turn into the distance instead of
sliding a flat picture sideways:

```text
heading[n+1] = heading[n] + curvature[n]
x[n+1]       = x[n] + heading[n+1]
y[n+1]       = y[n] + grade[n]
```

Curvature and grade enter and leave with a smoothstep envelope. Discontinuous
curvature is forbidden because it creates a visible kink and an instantaneous
handling force.

## 3. Perspective projection

For a world point `P` and camera `C`, first translate to camera space:

```text
dx = P.x - C.x
dy = P.y - C.y
dz = P.z - C.z
```

Only points with positive `dz` are visible. With focal length `f`:

```text
scale    = f / dz
screen_x = screen_center_x + dx * scale
screen_y = horizon_y       - dy * scale
half_w   = road_half_world * scale
```

Road edges, props, dust, and rivals all use this same projection. A rival is
not assigned a hand-authored screen position: its `(s,e)` is converted through
the current road sample, then projected. That guarantees it follows curves and
hills without floating.

Road spans are generated far-to-near. A running highest-visible boundary clips
spans behind a hill crest. Without this occlusion rule, hidden road is painted
over the crest and elevation reads as transparent layers.

## 4. Longitudinal handling

Speed is integrated from separate forces rather than set from button state:

```text
v_next = clamp(v + drive - coast_drag - brake - surface_drag, 0, surface_cap)
s_next = s + v_next
```

Required relationships:

- throttle increases speed but drive force tapers toward the speed cap;
- released throttle produces coast drag;
- brake drag is stronger than coast drag for the same interval;
- off-road terrain reduces the attainable cap and adds drag;
- idle input can never create positive speed.

Exact tuning values are BAJANEW values selected through playtesting, not values
from a reference game.

## 5. Steering and curves

Raw left/right input selects a steering target. The steering state approaches
that target at a bounded rate and approaches zero faster after release:

```text
u_next = approach(u, input_target, input_rate_or_return_rate)
e_next = e + u_next * lateral_response(v) + curve_drift(v, curvature)
```

Low speed gives fine corrections; higher speed gives more authority without
making one-frame taps catastrophic. Curve drift is opposite the turn and grows
with the square of normalized speed. The player must steer through a fast bend,
but the force is bounded and recoverable.

## 6. Surface model

The road edge is evaluated in world space using `abs(e)`. A narrow shoulder
transition may mark one wheel off; beyond it both wheels are off. Surface state
controls:

- speed cap and extra drag;
- lateral grip;
- bounded suspension roughness and dust emission;
- recovery behavior near the edge.

Visual shake never changes course position. It is derived from deterministic
state so it cannot act as hidden steering input.

## 7. Rival AI

Every rival owns its own `(s,e,v)`, preferred pace, lane target, personality,
decision cooldown, collision cooldown, and overtake state.

Each AI step is split into perception, decision, and actuation:

1. Perception measures world-space forward gap and lateral overlap against the
   player and other rivals.
2. Decision chooses a target speed and target lane with hysteresis. A lane is
   never changed merely because a sprite crossed a screen pixel.
3. Actuation approaches both targets at bounded rates.

The Ensenada slice uses two intentionally different profiles:

- an evasive rival chooses open space and protects momentum;
- a blocking rival shadows the player's lane only inside a bounded interaction
  window and sacrifices some speed to do so.

Neither may teleport. Lane targets persist long enough to make motion readable.

## 8. Collision and overtaking

Broad-phase collision uses longitudinal overlap; narrow-phase uses lateral
overlap. Both tests operate in world space:

```text
abs(rival.s - player.s) < combined_half_length
abs(rival.e - player.e) < combined_half_width
```

A confirmed collision reduces speed according to relative motion, pushes the
player away from the contact side, starts a cooldown, and emits a visible event.
Non-overlap cannot collide. Position is computed from course distance, while an
overtake is a persistent ahead-to-behind crossing rather than a screen-space
disappearance.

## 9. Rendering contract

The renderer receives an immutable simulation snapshot and produces:

- projected road spans with left/right edges and depth;
- projected rivals with depth-derived scale and sort priority;
- player pose from steering, surface, collision, and suspension state;
- HUD values from speed, position, stage progress, and race phase.

The Neo Geo renderer may quantize these values to sprite tiles and zoom steps,
but it may not change the underlying simulation to make a screenshot line up.

## 10. Proof before art integration

The programming foundation is accepted only when automated tests prove idle
rest, throttle/coast/brake ordering, opposite steering, recentering, curved-road
projection, off-road penalty, rival independence, overtaking, collision and
non-collision, legal race phases, and deterministic replay. MAME then proves
real input and moving frames. Greg's five-minute unscripted play remains the
release verdict.
