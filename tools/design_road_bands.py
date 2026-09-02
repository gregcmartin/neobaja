#!/usr/bin/env python3
"""Design the BAJANEW road band table.

The funnel is fixed: a band covers a constant depth interval, so its projected
half width never changes.  Only its screen row (elevation), horizontal centre
(curve and camera offset) and surface phase move.

A band is drawn by translating one authored strip, and a translation cannot
shear.  The true road edge inside a band is a straight line through the
vanishing point, so when the camera sits off the road's centre line the strip's
edge is wrong by (offset * rows / 3) pixels across the band.  That error is
proportional to the band's height in screen rows and nothing else, which is why
the near bands are capped at NEAR_ROWS rows while the far half of the funnel
spreads geometrically so the surface phases stream evenly toward the camera.

Prints the C tables consumed by src/sim.c and the strip generator.
"""
import math

HORIZON_Y = 84
BOTTOM_Y = 224
CAM_HEIGHT = 3.0
FOCAL = 160.0
ROAD_HALF = 4.0
DEPTH_C = CAM_HEIGHT * FOCAL          # depth = DEPTH_C / dy

FAR_BANDS = 15                        # geometric bands from DY_MIN to NEAR_DY
DY_MIN = 2.0
NEAR_DY = 48                          # where the strips reach full screen width
NEAR_ROWS = 12                        # cap on a near band's height in rows
# Metres of road across which one surface phase holds.  Never shorter than a
# band's own depth span, or the band would carry more than one phase at once.
MIN_HALF_WAVE_M = 8.0


def edges() -> list[int]:
    ratio = (NEAR_DY / DY_MIN) ** (1.0 / FAR_BANDS)
    far = [DY_MIN * ratio ** i for i in range(FAR_BANDS + 1)]
    out: list[int] = []
    for value in far:
        rounded = int(round(value))
        if not out or rounded > out[-1]:
            out.append(rounded)
    total = BOTTOM_Y - HORIZON_Y
    near_count = int(math.ceil((total - out[-1]) / NEAR_ROWS))
    step = (total - out[-1]) / near_count
    for i in range(1, near_count + 1):
        out.append(int(round(NEAR_DY + step * i)))
    out[-1] = total
    return out


def main() -> None:
    dy = edges()
    bands = len(dy) - 1
    print(f"// {bands} bands, edges {dy}")
    half = []
    depth = []
    scale = []
    edge_scale = []
    edge_depth = []
    shifts = []
    for b in range(bands):
        dy0, dy1 = dy[b], dy[b + 1]
        dym = (dy0 + dy1) / 2.0
        z = DEPTH_C / dym
        z_far, z_near = DEPTH_C / dy0, DEPTH_C / dy1
        half.append(int(round(ROAD_HALF * FOCAL / z)))
        depth.append(int(round(z * 65536)))
        scale.append(int(round(FOCAL / z * 65536)))
        span = z_far - z_near
        half_wave = MIN_HALF_WAVE_M
        while half_wave < span:
            half_wave *= 2.0
        shifts.append(16 + int(round(math.log2(half_wave))))
    for i in range(bands + 1):
        # The projection samples each boundary at the depth between its two
        # bands' centres, so the boundary scale is the one at that depth.
        if i == 0:
            z = DEPTH_C / dy[0] * 1.1
        elif i == bands:
            z = DEPTH_C / ((dy[bands - 1] + dy[bands]) / 2.0) * 0.9
        else:
            z = (DEPTH_C / ((dy[i - 1] + dy[i]) / 2.0) + DEPTH_C / ((dy[i] + dy[i + 1]) / 2.0)) / 2.0
        edge_scale.append(int(round(FOCAL / z * 65536)))
        edge_depth.append(int(round(z * 65536)))

    def table(ctype: str, name: str, values: list[int], size: str) -> str:
        body = ", ".join(str(v) for v in values)
        lines = [f"{ctype} {name}[{size}] = {{"]
        row = "   "
        for v in values:
            token = f" {v},"
            if len(row) + len(token) > 78:
                lines.append(row)
                row = "   "
            row += token
        lines.append(row.rstrip(","))
        lines.append("};")
        return "\n".join(lines)

    print(f"#define BAJA_ROAD_BANDS {bands}")
    print(table("const int16_t", "baja_band_dy", dy, "BAJA_ROAD_BANDS + 1"))
    print(table("const int16_t", "baja_band_half_width", half, "BAJA_ROAD_BANDS"))
    print(table("const uint8_t", "baja_band_stripe_shift", shifts, "BAJA_ROAD_BANDS"))
    print(table("static const BajaFp", "band_depth_fp", depth, "BAJA_ROAD_BANDS"))
    print(table("static const BajaFp", "band_scale_fp", scale, "BAJA_ROAD_BANDS"))
    print(table("static const BajaFp", "band_edge_scale_fp", edge_scale, "BAJA_ROAD_BANDS + 1"))
    print(table("static const BajaFp", "band_edge_depth_fp", edge_depth, "BAJA_ROAD_BANDS + 1"))
    print(f"{'b':>3} {'dy0':>4} {'dy1':>4} {'rows':>4} {'z':>7} {'halfw':>5} {'shift':>5}")
    for b in range(bands):
        print(f"{b:>3} {dy[b]:>4} {dy[b + 1]:>4} {dy[b + 1] - dy[b]:>4} "
              f"{depth[b] / 65536:>7.2f} {half[b]:>5} {shifts[b]:>5}")


if __name__ == "__main__":
    main()
