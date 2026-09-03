"""Key and cut the Grok Build raws under art/raw/grok into sprite cut-outs.

Every raw stays byte-for-byte as Grok saved it.  The generator paints its
"transparent" background as a checkerboard or a flat sky, so the background is
recovered by flood filling from the image's edge through background-looking
pixels only: pixel-art outlines stop the fill at the subject.
"""
from __future__ import annotations

from collections import deque

import numpy as np
from PIL import Image

from bajaart import ROOT

GROK_RAW = ROOT / "art/raw/grok"


def _checker(rgb: np.ndarray) -> np.ndarray:
    low = rgb.min(axis=2)
    high = rgb.max(axis=2)
    return (low >= 170) & ((high - low) <= 24)


def _sky(rgb: np.ndarray) -> np.ndarray:
    """Saturated sky blue; pale blue-white stripes on a canopy stay."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return (b >= 150) & (b > r + 45) & (b >= g - 10) & (r < 190)


def _border_colours(rgb: np.ndarray, tolerance: int = 40) -> np.ndarray:
    """Pixels close to the colours that dominate the image's outer edge.

    A checkerboard's two greys and a flat sky both live on the border; the
    subject rarely does.  Colours are clustered coarsely and the clusters that
    cover most of the border become background candidates."""
    height, width, _ = rgb.shape
    border = np.concatenate([rgb[0], rgb[-1], rgb[:, 0], rgb[:, -1]], axis=0)
    coarse = (border // 24) * 24 + 12
    unique, counts = np.unique(coarse, axis=0, return_counts=True)
    order = np.argsort(-counts)
    total = counts.sum()
    centres = []
    covered = 0
    for index in order:
        centres.append(unique[index])
        covered += counts[index]
        if covered >= total * 0.92 or len(centres) >= 4:
            break
    out = np.zeros((height, width), dtype=bool)
    for centre in centres:
        distance = np.abs(rgb - centre[None, None, :]).max(axis=2)
        out |= distance <= tolerance
    return out


def _enclosed_pockets(rgb: np.ndarray, pockets: np.ndarray, strict: np.ndarray | None,
                      bimodal: bool) -> np.ndarray:
    """Background pockets the edge flood could not reach - between a wheel and
    a body, under an awning.  A checker pocket shows two tones in quantity
    (bimodal luminance); a sky pocket is mostly strict sky.  A flat body
    panel is one tone and stays."""
    height, width = pockets.shape
    lum = rgb.sum(axis=2) / 3.0
    seen = np.zeros_like(pockets)
    out = np.zeros_like(pockets)
    for sy in range(height):
        for sx in range(width):
            if not pockets[sy, sx] or seen[sy, sx]:
                continue
            members = []
            queue = deque([(sy, sx)])
            seen[sy, sx] = True
            while queue:
                y, x = queue.popleft()
                members.append((y, x))
                for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
                    if 0 <= ny < height and 0 <= nx < width and pockets[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        queue.append((ny, nx))
            if len(members) < 100:
                continue
            ys = np.array([m[0] for m in members])
            xs = np.array([m[1] for m in members])
            if bimodal:
                values = lum[ys, xs]
                middle = np.median(values)
                high = (values > middle + 10).mean()
                low = (values < middle - 10).mean()
                if high >= 0.2 and low >= 0.2:
                    out[ys, xs] = True
            elif strict is not None and strict[ys, xs].mean() >= 0.4:
                out[ys, xs] = True
    return out


def _magenta(rgb: np.ndarray) -> np.ndarray:
    """The flat magenta a repaint put behind a subject, JPEG ringing included."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return (r >= 140) & (b >= 90) & (g <= 100) & (r > g + 80)


def _magenta_soft(rgb: np.ndarray) -> np.ndarray:
    """Magenta plus the pink fringe a glowing subject blends into it."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return (r >= 150) & (b >= 80) & (g <= 140) & (r > g + 50) & (b > g)


def _flood(mask: np.ndarray, seeds: list[tuple[int, int]]) -> np.ndarray:
    """Background pixels reachable from the seeds through background pixels."""
    height, width = mask.shape
    out = np.zeros_like(mask)
    queue: deque[tuple[int, int]] = deque()
    for y, x in seeds:
        if 0 <= y < height and 0 <= x < width and mask[y, x] and not out[y, x]:
            out[y, x] = True
            queue.append((y, x))
    while queue:
        y, x = queue.popleft()
        for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if 0 <= ny < height and 0 <= nx < width and mask[ny, nx] and not out[ny, nx]:
                out[ny, nx] = True
                queue.append((ny, nx))
    return out


def key_raw(name: str, background: str, seeds: str = "all") -> np.ndarray:
    """RGBA cut-out of a raw, cropped to its subject.

    background: "checker", "sky", or "magenta" for a raw Grok repainted onto a
    flat magenta field.  seeds: "all" edges, or "top" when the subject stands
    on ground that reaches the bottom edge.
    """
    path = next(p for p in sorted(GROK_RAW.iterdir()) if p.stem == name)
    rgb = np.array(Image.open(path).convert("RGB")).astype(np.int32)
    height, width, _ = rgb.shape
    if background == "checker":
        candidate = _checker(rgb) | _border_colours(rgb)
    elif background == "checker_strict":
        # The subject reaches the image edge, so the edge's own colours must
        # not be taken for background: the checkerboard greys only.
        candidate = _checker(rgb)
    elif background == "magenta":
        candidate = _magenta(rgb)
    elif background == "magenta_soft":
        candidate = _magenta_soft(rgb)
    else:
        candidate = _sky(rgb)
    points = [(0, x) for x in range(0, width, 8)]
    if seeds == "all":
        points += [(height - 1, x) for x in range(0, width, 8)]
        points += [(y, 0) for y in range(0, height, 8)] + [(y, width - 1) for y in range(0, height, 8)]
    else:
        points += [(y, 0) for y in range(0, height // 2, 8)] + [(y, width - 1) for y in range(0, height // 2, 8)]
    background_mask = _flood(candidate, points)
    if background in ("checker", "checker_strict"):
        # A painted shadow tints the checkerboard under a subject; the loose
        # test admits those pockets and the two-tone rule keeps body panels.
        loose = (rgb.min(axis=2) >= 120) & ((rgb.max(axis=2) - rgb.min(axis=2)) <= 48)
        background_mask |= _enclosed_pockets(rgb, loose & ~background_mask, None, True)
    elif background == "magenta":
        background_mask |= _enclosed_pockets(rgb, candidate & ~background_mask, candidate, False)
    else:
        # Sky fades pale toward the horizon; a pocket of it under an awning
        # is still mostly sky.
        r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
        loose = (b >= 150) & (b >= r + 10) & (b >= g - 10)
        # Only pale sky counts toward accepting a pocket: a blue shirt is
        # sky-coloured but never that light.
        pale_sky = candidate & (b >= 205)
        background_mask |= _enclosed_pockets(rgb, loose & ~background_mask, pale_sky, False)
    alpha = np.where(background_mask, 0, 255).astype(np.int32)
    rgba = np.dstack([rgb, alpha])
    ys, xs = np.where(alpha > 0)
    if len(xs) == 0:
        raise SystemExit(f"{name}: keying removed everything")
    return rgba[ys.min():ys.max() + 1, xs.min():xs.max() + 1]


def trim_ground(rgba: np.ndarray) -> np.ndarray:
    """Drop a flat ground slab the generator painted under a prop.

    A slab spans nearly the whole cut-out's width with a straight bottom edge;
    the prop itself never does.  Rows are removed from the bottom while their
    opaque span covers most of the width, then the cut-out is re-cropped."""
    alpha = rgba[..., 3] >= 128
    width = rgba.shape[1]
    bottom = rgba.shape[0]
    while bottom > 1:
        row = np.where(alpha[bottom - 1])[0]
        if len(row) == 0:
            bottom -= 1
            continue
        if row.max() - row.min() + 1 < width * 0.85:
            break
        bottom -= 1
    out = rgba[:bottom].copy()
    ys, xs = np.where(out[..., 3] >= 128)
    return out[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
