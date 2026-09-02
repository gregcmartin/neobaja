# OpenAI image-generation prompts

Provider: OpenAI built-in `image_gen`

Generation group: `01a05452-0194-7532-93b8-bca5bd770d7c`

## Player vehicle

Original late-1990s arcade pixel-art sprite sheet of a large blue fictional
desert racing side-by-side from a low rear chase camera. Five consistent poses:
neutral, steer-left, steer-right, compression, and rebound/airborne. Transparent
background; detailed cage, spare, shocks, suspension, tires, exhaust, shadow;
no brands, words, logos, or reference pixels.

## Rival vehicle

Original orange-and-black fictional compact side-by-side rival in four
increasing depth scales plus an evasive pose. Rear camera, transparent
background, distinct cage/body/light geometry, no brands or text. A follow-up
background-extraction request removed the provider's first brown backdrop.

## Ensenada horizon

Original bright Pacific coastal far-background layer: turquoise water, ochre
cliffs, layered blue mountains, clouds and a tiny unbranded support helicopter.
No road, vehicle, HUD, signs, words, logos, or gameplay composition.

## Max and Cruz

Two fictional child racer portraits: Max, age 8, blond, lighter complexion,
blue/gold number 8 jersey; Cruz, age 6, brown hair, slightly darker complexion,
orange/blue number 6 jersey. A follow-up changed only Cruz to a short-haired boy
and requested transparent background. No real-person likenesses or branding.

### Outfit and number revision

Use case: precise-object-edit. Change only the racers' outfit colors and chest
numbers. The left blond racer Max must wear a predominantly red racing outfit
and display the exact number `2`; the right brown-haired racer Cruz must wear a
predominantly blue racing outfit and display the exact number `17`. Preserve
their faces, identities, ages, hair, skin tones, expressions, anatomy, outlines,
poses, hands, helmets, gloves, belts, scale, crop, spacing, pixel-art style and
background. Keep Max left and Cruz right. No logos, brands, extra characters,
other text, number 8, number 6, orange-dominant Cruz outfit, or blue-dominant
Max outfit.

## Ensenada terrain and props

Original 4x4 modular atlas containing four dirt/shoulder textures, rocks, agave,
scrub, palm, cactus, chevron, blue flag, spectators, dust and pebbles. True
transparent background, consistent Pacific daylight, no words or logos.

## Ensenada full environment

Verbatim final prompt:

> Create a genuinely original, legally distinct late-1990s premium 2D arcade
> pixel-art environment sheet for a fictional off-road racing game on Neo
> Geo-class hardware. Native gameplay composition is 320x224, low rear chase
> camera, but output at high resolution for deterministic downsampling. Scene:
> a sunlit Pacific coast near Ensenada, Mexico, with a broad ochre dirt racing
> track starting very wide at the bottom and narrowing strongly to a vanishing
> point just above mid-screen; continuous readable road edges; twin tire ruts;
> loose dust and small stones; dense layered rocky shoulder; agave, cactus,
> scrub and one wind-bent palm; turquoise ocean and surf visible down the left
> coastline; ochre cliffs and layered blue mountains in the distance; a small
> unbranded support helicopter; a modest group of tiny spectators safely high
> on the right shoulder; cinematic depth from foreground rocks to distant
> coast. Make the lower 60% a rich continuous racing surface and roadside
> environment, not a tile sheet and not a grid. Leave the driving line open. No
> vehicles, no HUD, no gauges, no text, no words, no logos, no brands, no signs,
> no flags, no copyrighted characters, no visual quotation of any existing
> game. Crisp hand-authored pixel clusters, warm sun, saturated
> cyan/ochre/green/navy palette, high detail but strong readability at 320x224.
> Single full-frame environment, straight road, centered vanishing point.


# Grok Build prompts (2026-09-02)

Provider: Greg's Grok Build account, model grok-4.6, Imagine `image_gen` tool,
run headlessly from this project with `grok -p`.  Raw outputs are saved
byte-for-byte under `art/raw/grok/` (JPEG as the tool produced them; the test
tyre stack was converted to PNG inside the Grok session).  Hashes are in
`art/raw/grok/SHA256SUMS`.  Every prompt below ended with the shared style
clause: "late-1990s Neo Geo arcade pixel art, crisp hard-edged pixels, limited
palette, bright Baja daylight, isolated on a plain transparent background, no
words, no logos, no brands".

## Title logo (`logo.jpg`, 16:9)

Original arcade racing game title logo that reads exactly BAJA on the first
line and OUTRUN on the second line, chunky bevelled letters with a chrome and
sunset-orange gradient, a thick dark outline and a hard drop shadow, a stylised
sun and a dust trail flourish behind the letters, late-1990s Neo Geo arcade
pixel art, isolated on a plain transparent background, no other words, no
logos, no brands.

## Checkpoint arch (`arch.jpg`, 16:9)

A wide inflatable race checkpoint arch, blue with a blank white banner strip
across the top and orange stripes on the pillars, seen straight on from the
front.  The banner is lettered START / FINISH by the converter.

## Billboard (`billboard.jpg`, 1:1)

A rustic roadside billboard: two weathered wooden posts and a wooden frame
holding a blank cream-coloured board face with a slight shadow along its top
edge, seen straight on.  The face is lettered by the converter.

## Pit awning (`pit_tent.jpg`, 1:1)

A small off-road race pit awning: a white and blue canopy on four poles with a
folding table, red tool boxes and two crew members in blue shirts standing
beneath it, seen from the front slightly above.

## Spectators (`spectators.jpg`, 1:1)

A tight cluster of six cheering spectators at a desert race, hats, sunglasses,
one waving a blue flag and one holding a striped umbrella, seen from the front
at ground level.

## Support helicopter (`helicopter.jpg`, 1:1)

A small white and red support helicopter in side view flying to the right with
a blurred main rotor and tail rotor, seen level from the side.

## Background repaints (`*_keyed.jpg`)

`pit_tent_keyed.jpg`, `spectators_keyed.jpg`, `billboard_keyed.jpg` and
`boulders_keyed.jpg` are `image_edit` passes over the raws above with this
prompt: "Replace the entire background behind the subject, including any sky
and any ground or sand patch the subject stands on, with one flat solid
magenta colour, RGB 255 0 255, right up to the subject's outline.  Keep the
subject itself exactly as it is, unchanged in colour, shape and detail."  The
painted sky could not be keyed from the subjects' pale stripes and blue
shirts; a flat magenta field can.  The converter reads the repaints.

## Rival buggy (`rival_maverick.jpg`, 1:1)

Rear view only: an original red and white compact off-road side-by-side racing
buggy seen from directly behind, perfectly symmetrical, tail lights and twin
exhausts facing the viewer, a black roll cage, oversized knobby tyres, a
rear-mounted spare wheel and a roof light bar, low rear chase camera angle as
in a 1990s arcade racing game, the buggy floats with no ground, no shadow and
no dust beneath it, no numbers.  (A first attempt came out at a three-quarter
angle and a second carried a painted ground shadow with the checkerboard
showing through it; both were discarded.)

## Boulders (`boulders.jpg`, 1:1)

A cluster of three large weathered sandstone boulders with tufts of dry desert
grass at their base, seen from the front at ground level.

## Oil drums (`drums.jpg`, 1:1)

Three dented steel oil drums painted orange and white, two standing and one
lying across them, seen from the front at ground level.

## Tyre stack (`test_tyres.png`, 1:1)

A small original pixel-art icon of a wooden Baja race tyre stack, transparent
background.  This was the first headless generation test and shipped as the
tyre-stack prop.
