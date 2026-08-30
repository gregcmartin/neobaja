# BAJA: Edge of Control HD visual quality reference

These eight screenshots are a private, non-shipping visual benchmark for the
BAJA Outrun Gauntlet. They were downloaded from the official Steam store API
for app `623090` on 2026-08-30. They are copyrighted by their respective
owners and are ignored by this project's `.gitignore` through
`/art/workbench/`; do not package them in the ROM or a release.

Official store page:
<https://store.steampowered.com/app/623090/BAJA_Edge_of_Control_HD/>

## Pinned reference files

| File | Official Steam CDN source | SHA-256 |
| --- | --- | --- |
| `01-race-dust.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_fa7160c72ebd8dd9526907e4db06f337adc9b470.1920x1080.jpg?t=1727857574` | `8cb04bdefed5c880e98a8882446a4903f523be118828815bf4798d220db57010` |
| `02-race-pack.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_a40415fd03f5a4433862079d737fd02b4402ec9f.1920x1080.jpg?t=1727857574` | `d20e5fdf396f8cecda979fee021eeac63460b26347b53282c2ce174dfca30d1d` |
| `03-terrain.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_976544d9b7b090ea23aa41eb410900900f53c145.1920x1080.jpg?t=1727857574` | `3eb7d0fe6265bd57860e9b0fc57da016540173cd7bc15c6f0882142591822c2a` |
| `04-canyon.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_58939970b6bb4a6e2b917f5b5688222b6dbe0e88.1920x1080.jpg?t=1727857574` | `16254c6fdfed9459ab5267e06d7b92015620ba8bc1855a26c4f72f5b6b1a0914` |
| `05-jump.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_ee6986f5ce96f8599bd61c2ba8155a68363c42aa.1920x1080.jpg?t=1727857574` | `0c83787fa93094f37dab05903c18973742b0c76b7a2257524b1cd4cfa9facb67` |
| `06-landscape.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_e21424cbdf4d6ff162524e7ae330603f9d7a6f51.1920x1080.jpg?t=1727857574` | `e74e05c9e0b03ae1c10f56566ad5a859c581e171f2cd9d82a4627779817dc541` |
| `07-hillclimb.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_fc88160f8df542be4a96877c831a41ac8bac1906.1920x1080.jpg?t=1727857574` | `f45311e74c7f90bfa426a09c01ddf81ed64d09d76260373d5725927041df3bec` |
| `08-terrain-variation.jpg` | `https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/623090/ss_bd5bd8f861202edf1de949efa5e5ea46f170e300.1920x1080.jpg?t=1727857574` | `e98df25d798ffcd98d7fb992cfe40be178f8f564bae3bf7fb448ca18cec425f7` |

All files are 1920x1080 JPEGs. Verify the hashes before each Gauntlet run.

## Neo Geo translation rubric

The references are a quality and composition bar, not source material. Do not
copy their vehicles, liveries, logos, camera frames, textures, or layouts.
Judge the live 320x224 MAME captures against these qualities within Neo Geo
hardware limits:

1. **Vehicle presence and identity** — the player vehicle is a large, readable
   focal object with convincing cage, suspension, tires, lights, body planes,
   and distinct steering/impact poses. Rivals remain recognizable at depth.
2. **Depth and race composition** — foreground, mid-pack, horizon, road
   convergence, scale tiers, and occlusion create a busy off-road race rather
   than isolated icons on a flat strip.
3. **Terrain specificity** — visible ruts, berms, rocks, scrub, cactus,
   elevation, and layered landforms make the course feel driven through and
   distinguish the four named Baja stages.
4. **Motion and physicality** — dust originates at tires, suspension/vehicle
   poses imply bumps and steering, shadows ground vehicles, and scrolling or
   animation changes across captured frames.
5. **Lighting and palette craft** — dramatic warm/cool separation, readable
   silhouettes, controlled highlight clusters, and atmospheric depth survive
   15-color indexed palettes without mush or soft-resampled pixels.
6. **Density with clarity** — scenery, rivals, effects, HUD, and signage are
   richly layered while the racing line and player remain immediately legible
   at native 1x.
7. **Late-era Neo Geo finish** — custom pixel clusters, coherent outlines,
   purposeful dithering, animation consistency, clean transparency, and no
   placeholder geometry or generic generated-image residue.

Each visual critic must score every category from 0 to 4 using an actual MAME
capture and cite visible evidence. PASS requires every category at least 3 and
at least 24/28 overall. A score without opening both the reference images and
the current MAME captures is invalid. If a round fails, its lowest-scoring
category becomes the next repair target and fresh agents must re-score new
captures after the repair.
