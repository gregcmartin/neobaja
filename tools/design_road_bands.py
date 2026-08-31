#!/usr/bin/env python3
"""Design the BAJANEW road band table.

The funnel is fixed: a band covers a constant depth interval, so its projected
half width never changes.  Only its screen row (elevation) and surface phase
move.  This script picks the band boundaries and prints the C table consumed by
src/sim.c and the strip generator.
"""
import math

HORIZON_Y = 84
BOTTOM_Y = 224
CAM_HEIGHT = 3.0
FOCAL = 160.0
ROAD_HALF = 4.0
BANDS = 16

DY_MIN = 2.0
DY_MAX = float(BOTTOM_Y - HORIZON_Y)  # 140

# Geometric progression in dy keeps every band a similar depth ratio, which is
# what makes the surface phases stream evenly toward the camera.
ratio = (DY_MAX / DY_MIN) ** (1.0 / BANDS)
edges = [DY_MIN * ratio ** i for i in range(BANDS + 1)]
edges = [round(e) for e in edges]
for i in range(1, len(edges)):
    if edges[i] <= edges[i - 1]:
        edges[i] = edges[i - 1] + 1
edges[-1] = BOTTOM_Y - HORIZON_Y

print(f"ratio={ratio:.4f}")
print(f"{'band':>4} {'dy0':>4} {'dy1':>4} {'rows':>4} {'depth':>8} {'halfw':>6} {'stripw':>6} {'cols':>4} {'tilerows':>8}")
total_cols = 0
rows = []
for b in range(BANDS):
    dy0, dy1 = edges[b], edges[b + 1]
    dym = (dy0 + dy1) / 2.0
    depth = CAM_HEIGHT * FOCAL / dym
    halfw = ROAD_HALF * FOCAL / depth
    # terrain margin: one road width of dirt on each side, capped to a strip
    # wide enough to survive any on-screen lateral offset.
    strip = min(640, int(round(halfw * 2 * 3)))
    strip = max(16, (strip + 15) // 16 * 16)
    cols = strip // 16
    onscreen = min(20, cols)
    band_rows = dy1 - dy0
    tile_rows = max(1, (int(math.ceil(band_rows * 1.75)) + 15) // 16)
    total_cols += onscreen
    rows.append((b, dy0, dy1, band_rows, depth, halfw, strip, cols, tile_rows))
    print(f"{b:>4} {dy0:>4} {dy1:>4} {band_rows:>4} {depth:>8.2f} {halfw:>6.1f} {strip:>6} {cols:>4} {tile_rows:>8}")

print(f"on-screen columns for the road: {total_cols}")
print(f"strip tiles (2 phases): {sum(r[7]*r[8] for r in rows) * 2}")
print()
print("static const int16_t band_dy[BAJA_ROAD_BANDS + 1] = {" + ", ".join(str(e) for e in edges) + "};")
print("static const int16_t band_half_width[BAJA_ROAD_BANDS] = {" + ", ".join(str(int(round(r[5]))) for r in rows) + "};")
print("static const int32_t band_depth_fp[BAJA_ROAD_BANDS] = {" + ", ".join(str(int(round(r[4] * 65536))) for r in rows) + "};")
