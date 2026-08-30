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
