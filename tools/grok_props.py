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
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return (b >= 120) & (b > r + 25) & (b >= g - 10) & (r < 200)


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

    background: "checker" or "sky".  seeds: "all" edges, or "top" when the
    subject stands on ground that reaches the bottom edge.
    """
    path = next(p for p in sorted(GROK_RAW.iterdir()) if p.stem == name)
    rgb = np.array(Image.open(path).convert("RGB")).astype(np.int32)
    height, width, _ = rgb.shape
    candidate = _checker(rgb) if background == "checker" else _sky(rgb)
    candidate |= _border_colours(rgb)
    points = [(0, x) for x in range(0, width, 8)]
    if seeds == "all":
        points += [(height - 1, x) for x in range(0, width, 8)]
        points += [(y, 0) for y in range(0, height, 8)] + [(y, width - 1) for y in range(0, height, 8)]
    else:
        points += [(y, 0) for y in range(0, height // 2, 8)] + [(y, width - 1) for y in range(0, height // 2, 8)]
    background_mask = _flood(candidate, points)
    alpha = np.where(background_mask, 0, 255).astype(np.int32)
    rgba = np.dstack([rgb, alpha])
    ys, xs = np.where(alpha > 0)
    if len(xs) == 0:
        raise SystemExit(f"{name}: keying removed everything")
    return rgba[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
