# User Neo Geo gameplay mockups — primary visual bar

These four user-supplied images are the **primary visual references** for the
BAJA Outrun Gauntlet. They are inspection inputs, not shipping assets. Keep the
PNG files byte-for-byte unchanged and never compile, trace, crop, repaint, or
redistribute them as game content.

The official BAJA: Edge of Control HD reference pack in
`../baja-edge-of-control-hd/` is secondary. Use it only for real-world off-road
vehicle, terrain, dust, suspension, and race-composition cues. When references
conflict, these user mockups control the desired Neo Geo presentation while the
root README controls game identity and behavior.

## Pinned files

All source files were supplied at 1448x1086 RGB PNG (4:3 display aspect).
BAJA Outrun must still render natively at 320x224; treat the mockups as a
display-stretched art-direction target, not a request to change native output.

| Local reference | Original path | SHA-256 | Intended stage translation |
|---|---|---|---|
| `01-bahia-night-storm.png` | `/Users/gregmartin/Desktop/349f2f77-90c3-46e4-8616-04f1d9dd33ba.png` | `9f39c438479e9a6501ccfeac6fb1f408269a39c74e0803bfbae338ffb9843cda` | Bahia de los Angeles: moonlit cobalt storm, lightning, checkpoint lights |
| `02-valle-canyon-jump.png` | `/Users/gregmartin/Desktop/03d265b3-a935-4f69-9c37-338e15fc93f0.png` | `1f96f3a06c00cea5c563406bf532d656a271f5c266a032571929e1a2602d029e` | Valle de Trinidad: orange canyon sunset, mesa depth, crest/jump |
| `03-san-felipe-desert-pack.png` | `/Users/gregmartin/Desktop/000e4dd1-16e3-4939-a3ab-60dfbea3ebde.png` | `f444fde3a1063354d9936842bc8518de49c6263d8730f2da54e341f282777a81` | San Felipe: open hot desert, long rutted straight, multi-rival pack |
| `04-ensenada-pacific-run.png` | `/Users/gregmartin/Desktop/f26cf451-72d3-48d2-953a-9a31905cd8fc.png` | `22f162a5c8e2132e4b75dde95830fae1ba2c58b4cb488f2159e9e16a48308b7c` | Ensenada: bright Pacific cliff road, blue water, helicopter/crowd energy |

## Frozen gameplay art bible

### Composition and camera

- Rear chase camera, low enough to make suspension travel and tire contact
  readable. Put the horizon/vanishing point near the upper-middle of the play
  lane, with an unmistakable road funnel from the distance to the bottom edge.
- The player 2022 Polaris RZR Pro R must be the visual anchor, approximately
  96–120 native pixels wide at its closest normal race scale and large enough
  to occupy roughly the lower third to lower half of the frame. It must never
  read as a tiny icon or a generic rectangle.
- Keep the center road and immediate hazards legible even with a dense HUD.
  Rivals must scale convincingly with depth and visibly occupy the same world.
- Use foreground rocks/brush, midground road/rivals/crowds, far terrain, and sky
  as distinct depth tiers. No single flat horizon strip.

### Vehicle identity and animation

- Preserve the README's blue 2022 Polaris RZR Pro R identity rather than
  copying the trophy trucks shown in the mockups. Translate their visual weight
  and mechanical specificity: roll cage, rear body panels, lights, spare tire,
  shocks, suspension arms, exhaust, tire tread, number/livery, and cast shadow.
- Rivals remain visibly different Polaris RZR and Can-Am Maverick models, with
  distinct silhouettes, liveries, light clusters, and cage/body geometry.
- Production atlases need coherent neutral, steer-left, steer-right, suspension
  compression, rebound/airborne, and damage/contact poses as gameplay permits.
  Add animated wheel/tire, shadow, dust, pebble, and rut interactions so the
  vehicle feels attached to the terrain.

### Environment, motion, and stage identity

- Roads need readable ruts, alternating grooves, berms, loose stones, color
  variation, and dust sources tied to tires. Road animation must communicate
  speed, not merely scroll a featureless brown band.
- Bahia: deep navy/cobalt sky, silver moon, lightning silhouette, warm pools of
  checkpoint light, wet/cool shadows, sparse crowd and cactus highlights.
- Valle: blazing orange sunset, canyon walls and mesas in multiple value tiers,
  a visible crest/jump, long shadows, amber dust, rock ledges, chevrons.
- San Felipe: expansive warm desert, high sun/late-gold sky, distant landmark,
  open rutted route, multiple rivals, spectators/signage, cactus and scrub.
- Ensenada: high-key blue daylight, Pacific coastline, pale clouds, cliff road,
  coastal vegetation, crowd/air-support spectacle, turquoise-versus-ochre color
  separation.
- Stage differentiation must survive grayscale/value inspection; swapping a
  sky color over the same terrain is not sufficient.

### Pixel craft and palette

- Author for native 320x224 and Neo Geo indexed sprite/tile limits. Use hard,
  intentional pixel clusters, selective outlines, controlled dithering, and
  15-color-plus-transparency tile banks. No soft resampling, anti-aliased halos,
  subpixel blur, baked fake scanlines, or arbitrary AI-image fragments.
- Use nearest-neighbor conversion and inspect at native 1x as well as integer
  scale. Preserve silhouettes first, then mechanical highlights and texture.
- Lighting must shape vehicles and terrain: bright edge planes, dark underbody,
  contact shadow, luminous dust, and palette ramps matched to each stage.
- The desired finish is dense, confident late-era Neo Geo pixel art. Do not copy
  pixels, proprietary vehicle art, logos, sponsor marks, or trade dress from
  any reference. Replace visible reference brands with original BAJA Outrun and
  Max Cruz Racing Team world-building.

### HUD and graphic language

- Follow the mockups' hierarchy: position/lap upper left; large time upper
  center; best/lap time upper right; tachometer/speed/gear lower left; stage
  name and readable minimap lower right.
- Recompose this hierarchy for 320x224 instead of shrinking the 1448x1086 HUD.
  Use compact outlined bitmap type, strong value contrast, consistent number
  forms, and safe margins. No illegible generated text in final source art.
- The title/select/countdown/finish screens have no direct screenshot here, so
  extrapolate the same palette discipline, outlined typography, dense desert
  framing, mechanical detail, and character/vehicle identity. The developer
  splash remains its separate exact five-second requirement.

## Mandatory critic protocol

Every contract, baseline, visual-critique, and final-approval agent must use an
image-capable read operation to open all four pinned mockups and the fresh MAME
captures. File existence, hashes, prose descriptions, contact-sheet metadata,
or model memory do not count as visual inspection.

For every required stage, compare its fresh native 320x224 MAME race capture
against the mapped mockup above. Record exact paths opened, visible agreements,
visible gaps, and a 0–4 integer score in every category:

1. composition, chase camera, depth, and readable road hierarchy;
2. player/rival vehicle scale, silhouette, model identity, and mechanical detail;
3. environment density, terrain specificity, and unmistakable stage identity;
4. motion/physicality: dust, ruts, suspension, steering, rivals, and speed cues;
5. native pixel clusters, palette ramps, lighting, contrast, and clean conversion;
6. HUD hierarchy, bitmap typography, minimap, timing data, and playfield clarity;
7. production completeness and cohesion across splash/title/select/race/finish.

Scoring anchors: `0` absent/broken, `1` placeholder or far below target, `2`
recognizable but materially below target, `3` credible production-quality Neo
Geo translation with only bounded gaps, `4` convincingly matches or exceeds the
reference's intent within actual hardware constraints.

Visual PASS requires **every category >=3 and total >=24/28**, plus all
functional, provenance, reproducibility, hardware-budget, and MAME gates. A
score cannot be raised for metadata or intent. The lowest visible category must
drive the next repair; launch a fresh specialized generation/implementation
agent, produce new evidence, then have fresh critics rescore. Repeat until PASS
or the explicit round cap. Never declare PASS from the old deterministic art.

