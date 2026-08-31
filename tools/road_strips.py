"""Build the perspective road band strips from the approved Ensenada plate.

The plate is a perspective view of a ground plane.  Rectifying it into a
top-down road texture and re-projecting that texture onto BAJANEW's own funnel
gives strips whose ruts, stones and shoulder wear are the approved artwork,
drawn at exactly the scale the projection expects.

Detail is depth dependent on purpose.  A metre of road near the camera crosses
the band in well under one frame, so near strips carry only what survives that
speed: the lengthwise ruts, the shoulder, and the colour of the surface.  Fine
speckle is kept for the middle distance, where it can actually be resolved.
"""
from __future__ import annotations

import re

import numpy as np
from PIL import Image

from bajaart import (FUNNEL_K, PLATE_K, PLATE_VX, PLATE_VY, RAW, ROAD_PHASES,
                     ROOT, TEX_LEN_M, TEX_U_SPAN, TEX_W, band_tables)

PLATE = "ensenada-full-environment.png"

# The plate's own near row and the depth it stands for.  Everything else is
# derived, so the rectification is one fit with no hidden constants.
PLATE_DY_NEAR = 660.0
PLATE_NEAR_DEPTH = 3.5
PLATE_DEPTH_C = PLATE_DY_NEAR * PLATE_NEAR_DEPTH
# Lateral reach available before a sample leaves the plate.
PLATE_HALF_REACH = 735.0
# Length blended across the wrap so the texture tiles without a visible seam.
WRAP_BLEND_M = 4.0
# No pixel of the road surface may fall below this luminance, and anything
# already flat black becomes dirt rather than being scaled up from nothing.
SHADOW_FLOOR = 48.0
SHADOW_TONE = (62.0, 40.0, 20.0)


def _plate() -> np.ndarray:
    return np.array(Image.open(RAW / PLATE).convert("RGB")).astype(np.float64)


def rectify_road_texture(rows: int = 384) -> tuple[np.ndarray, np.ndarray]:
    """Return (texture, depths) sampled top-down from the plate.

    Rows are spaced by the plate's own resolution, so the near end keeps the
    detail the plate actually holds instead of being averaged flat.
    """
    plate = _plate()
    height, width, _ = plate.shape
    total_len = TEX_LEN_M + WRAP_BLEND_M
    depths = PLATE_NEAR_DEPTH * np.exp(
        np.linspace(0.0, np.log((PLATE_NEAR_DEPTH + total_len) / PLATE_NEAR_DEPTH), rows))
    texture = np.zeros((rows, TEX_W, 3), dtype=np.float64)
    us = np.linspace(-TEX_U_SPAN, TEX_U_SPAN, TEX_W, endpoint=False) + TEX_U_SPAN / TEX_W

    for index, depth in enumerate(depths):
        dy_ideal = PLATE_DEPTH_C / depth
        # A sample that would leave the plate is taken from further up it
        # instead; only the deep off-road margin is affected.
        reach = PLATE_HALF_REACH / (PLATE_K * np.maximum(np.abs(us), 0.05))
        dy = np.minimum(dy_ideal, reach)
        y = np.clip(np.round(PLATE_VY + dy).astype(int), 0, height - 1)
        x = np.clip(np.round(PLATE_VX + us * PLATE_K * dy).astype(int), 0, width - 1)
        texture[index] = plate[y, x]

    # Sharpen the road edge.  The plate already changes here; lifting the
    # packed berm and dropping the loose dirt just beyond it makes the boundary
    # readable at 1x before the player crosses it.
    edge = np.abs(us)
    gain = np.ones_like(edge)
    gain[(edge >= 0.90) & (edge < 1.03)] = 1.20
    gain[(edge >= 1.03) & (edge < 1.24)] = 0.84
    texture *= gain[None, :, None]
    np.clip(texture, 0, 255, out=texture)

    # Lift the deepest shadows.  The plate's far verge carries near-black
    # scrub, and a strip's outer columns clamp onto it and smear it into solid
    # blocks; a sunlit Baja road has no true black in it anyway.  Hue is kept
    # and only the level is raised.
    luminance = texture.sum(axis=2) / 3.0
    lift = np.where(luminance < SHADOW_FLOOR,
                    SHADOW_FLOOR / np.maximum(luminance, 1.0), 1.0)
    texture *= lift[:, :, None]
    flat = luminance < 6.0
    texture[flat] = np.array(SHADOW_TONE, dtype=np.float64)
    np.clip(texture, 0, 255, out=texture)

    # Cross-fade the tail back over the head so v wraps cleanly.
    keep = int(np.searchsorted(depths, PLATE_NEAR_DEPTH + TEX_LEN_M))
    tail = rows - keep
    if tail > 4:
        blend = np.linspace(1.0, 0.0, tail)[:, None, None]
        head = texture[:tail]
        texture[:tail] = head * blend + texture[keep:keep + tail] * (1.0 - blend)
    return texture[:keep], depths[:keep]


def strip_geometry() -> list[dict]:
    """Pixel size of every band strip, tile aligned."""
    dy, half = band_tables()
    out = []
    for b in range(len(half)):
        dy0, dy1 = dy[b], dy[b + 1]
        rows = dy1 - dy0
        width = int(round(2.0 * TEX_U_SPAN * FUNNEL_K * dy1))
        width = max(80, min(896, (width + 15) // 16 * 16))
        # Author tall enough that a downhill stretch never opens a seam.
        height = max(16, (int(round(rows * 1.9)) + 15) // 16 * 16)
        depth_far = 480.0 / dy0
        depth_near = 480.0 / dy1
        out.append({
            "band": b, "dy0": dy0, "dy1": dy1, "rows": rows,
            "width": width, "height": height,
            "tiles_x": width // 16, "tiles_y": height // 16,
            "columns_on_screen": min(20, width // 16),
            "depth_span_m": round(depth_far - depth_near, 3),
            "depth_mid_m": round(480.0 / ((dy0 + dy1) / 2.0), 3),
            "phases": ROAD_PHASES if stripe_shifts()[b] else 1,
        })
    return out


def stripe_shifts() -> list[int]:
    """Per-band surface phase wavelength, read from the gameplay core."""
    text = (ROOT / "src/sim.c").read_text(encoding="utf-8")
    match = re.search(r"const uint8_t baja_band_stripe_shift\[[^\]]*\] = \{([^}]*)\}", text)
    if not match:
        raise SystemExit("cannot find baja_band_stripe_shift in src/sim.c")
    return [int(v) for v in match.group(1).replace("\n", " ").split(",") if v.strip()]


def render_strip(texture: np.ndarray, depths: np.ndarray,
                 geometry: dict) -> np.ndarray:
    """Draw one band strip: a perspective slice of the rectified road."""
    width, height = geometry["width"], geometry["height"]
    dy0, dy1 = geometry["dy0"], geometry["dy1"]
    out = np.zeros((height, width, 4), dtype=np.float64)
    xs = np.arange(width) - (width - 1) / 2.0
    base = depths[0]
    span = depths[-1] - depths[0]

    # How much road crosses one screen row here, in metres.  Near strips get a
    # wide filter because that road is genuinely a blur at racing speed.
    metres_per_row = geometry["depth_span_m"] / max(1, geometry["rows"])
    blur_rows = int(np.clip(len(depths) * metres_per_row / span * 1.5, 1, len(depths) // 3))

    for row in range(height):
        dy = dy0 + (dy1 - dy0) * (row + 0.5) / height
        half_width = FUNNEL_K * dy
        depth = 480.0 / dy
        v = base + ((depth - base) % span)
        index = int(np.searchsorted(depths, v))
        index = min(max(index, 0), len(depths) - 1)
        lo = max(0, index - blur_rows // 2)
        hi = min(len(depths), lo + max(1, blur_rows))
        line = texture[lo:hi].mean(axis=0)

        u = np.clip(xs / half_width, -TEX_U_SPAN * 0.999, TEX_U_SPAN * 0.999)
        col = np.clip(((u + TEX_U_SPAN) / (2.0 * TEX_U_SPAN) * TEX_W).astype(int),
                      0, TEX_W - 1)
        out[row, :, :3] = line[col]
        out[row, :, 3] = 255.0

    return out


def build_sheets() -> tuple[list[tuple[str, np.ndarray, dict]], dict]:
    """One strip per band, at its authored surface phase 0.

    The alternate phase is produced later as a palette shift of these exact
    pixels, so both phases stay inside one fifteen colour road palette.
    """
    texture, depths = rectify_road_texture()
    sheets = []
    for geometry in strip_geometry():
        strip = render_strip(texture, depths, geometry)
        sheets.append((f"road{geometry['band']:02d}",
                       np.clip(strip, 0, 255).astype(np.int32), geometry))
    report = {
        "plate": PLATE,
        "vanishing_point": [PLATE_VX, PLATE_VY],
        "half_width_pixels_per_row": PLATE_K,
        "near_depth_m": PLATE_NEAR_DEPTH,
        "texture_rows": int(len(depths)),
        "texture_columns": TEX_W,
        "texture_length_m": TEX_LEN_M,
        "lateral_span_road_half_widths": TEX_U_SPAN,
        "edge_accent": {"berm_gain": 1.20, "verge_gain": 0.84},
        "shadow_floor": SHADOW_FLOOR,
        "shadow_tone": list(SHADOW_TONE),
        "bands": [dict(g) for g in strip_geometry()],
    }
    return sheets, report
