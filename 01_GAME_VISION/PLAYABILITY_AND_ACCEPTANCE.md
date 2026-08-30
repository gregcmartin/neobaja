# Human-first playability and acceptance

## Gate 0: one-stage vertical slice

Do not build the complete game first. The initial target is one Ensenada slice
with a title, character selection, countdown, projected road, player RZR, one
real rival, off-road response, collision response, HUD, finish, and restart.
Greg must approve this slice before production expands to all four stages.

## Manual MAME gate

A normal human session, with no scripted input or hidden state manipulation,
must demonstrate all of the following:

1. The developer splash remains visible for five seconds and ignores input.
2. The title waits indefinitely for Start.
3. Character selection waits indefinitely and clearly changes between Max and
   Cruz.
4. The race waits through a visible countdown and starts from rest.
5. With no input, the vehicle stays at rest.
6. A builds speed smoothly; releasing A coasts; B brakes more strongly.
7. Brief steering taps produce small corrections, sustained input produces a
   larger move, and release visibly recenters steering.
8. Road texture and scenery make forward motion obvious at low, medium, and
   high speed.
9. The player can identify the road edge before crossing it.
10. Off-road travel is slower and rougher but recoverable.
11. A collision looks and feels like contact without pinballing the player.
12. At least one rival approaches, moves independently, interacts, can be
    overtaken, and changes position feedback.
13. The HUD remains readable without hiding the driving line.
14. The stage can be finished and restarted without automation.

The session should last at least five continuous minutes. Greg's rejection is
authoritative even if every automated gate passes.

## Immediate failure conditions

- A menu advances or a racer is chosen without the player's command.
- Idle input applies throttle, steering, braking, or confirm.
- The course reads as a static picture with sprites pasted over it.
- Forward motion is visible only in telemetry or by comparing nearly identical
  screenshots.
- A light steering tap throws the player off the road.
- Rivals visibly float, share one motion path, teleport, or exist only in data.
- The road and stage background use incompatible perspective.
- Generated text, seams, transparency holes, or rectangular image fragments
  appear during ordinary play.
- A test driver is tuned to pass a script while an unscripted player cannot
  understand or control the game.

## Automated support gates

Automation is valuable only after the manual loop is acceptable. It should
then verify relational behavior rather than copy constants from a reference:

- deterministic fixed-step replay for the same input sequence;
- throttle to a cap, brake stronger than coast, and no negative speed;
- opposite left/right response and recentering;
- speed-driven course/camera progression and curve response;
- measurable off-road penalty and recoverability;
- distinct independent rivals, approach, lane decision, overtake/order change,
  collision, and non-collision;
- legal countdown, running, finish, restart/next-stage lifecycle;
- native 320x224 output and Neo Geo sprite/palette/ROM budgets;
- consecutive MAME frames with genuinely changing road, vehicle, rival, and
  HUD state;
- deterministic asset conversion with honest provenance.

## Visual approval gate

Review actual motion first, then native screenshots. Compare each stage with
its mapped primary mockup for:

1. chase-camera composition, depth, and road hierarchy;
2. vehicle scale, silhouette, identity, and mechanical detail;
3. terrain density and unmistakable stage identity;
4. motion, dust, suspension, steering, rivals, and speed cues;
5. native pixel craft, palette, lighting, and clean transparency;
6. HUD clarity and typography;
7. cohesion across splash, title, select, race, and finish.

No numeric score can turn a bad play session into a PASS. Physical AES/MVS is
always reported separately from MAME and remains unverified until actual
hardware is tested.

