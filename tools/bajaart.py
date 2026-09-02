"""Shared helpers for the BAJANEW art conversion.

Everything here is deterministic: the same approved raws always produce the
same source sheets, and every step is recorded so the conversion can be
audited without rerunning it.
"""
from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
RAW = ROOT / "art/raw/openai/01a05452-0194-7532-93b8-bca5bd770d7c"
ASSETS = ROOT / "assets"
SOURCE = ASSETS / "source"

# Screen contract, mirrored from include/baja/sim.h.
SCREEN_W, SCREEN_H = 320, 224
HORIZON_Y = 84
CAM_HEIGHT = 3.0
FOCAL = 160.0
ROAD_HALF = 4.0
# Screen pixels of road half width per row below the horizon.
FUNNEL_K = ROAD_HALF / CAM_HEIGHT

# Ground-plane fit of the road plate the strips are rectified from: the Grok
# Build rutted road (art/raw/grok/road_plate.jpg).  The road vanishes at VP
# and one road half width spans PLATE_K pixels per row.
ROAD_PLATE = "grok:road_plate.jpg"
PLATE_VX, PLATE_VY, PLATE_K = 576.0, 72.0, 0.45
# Fit of the approved Ensenada environment plate, still the backdrop source.
ENV_VX, ENV_VY, ENV_K = 740.0, 383.0, 0.78

# Rectified road texture: lateral span in road half widths, and the length of
# one repeat in metres.
TEX_U_SPAN = 2.6
TEX_LEN_M = 8.0
TEX_W, TEX_H = 512, 64
ROAD_PHASES = 2
# Lateral reach of a band strip in road half widths.  Beyond it the static
# ground layer shows, so this is the trade between streaming verge and the
# hardware sprite columns the near bands cost.
STRIP_U_SPAN = 1.75
# Road pixels further out than this fraction of the half width are verge, and
# get their own palette entries so the surface phase can stream them.
VERGE_U = 0.95


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def band_tables() -> tuple[list[int], list[int]]:
    """Read the band geometry straight out of the gameplay core.

    The strips and the projection have to agree exactly, so the C table is the
    single source of truth and this raises if it ever drifts.
    """
    text = (ROOT / "src/sim.c").read_text(encoding="utf-8")

    def table(name: str) -> list[int]:
        match = re.search(
            r"const int16_t " + name + r"\[[^\]]*\] = \{([^}]*)\}", text)
        if not match:
            raise SystemExit(f"cannot find {name} in src/sim.c")
        return [int(v) for v in match.group(1).replace("\n", " ").split(",") if v.strip()]

    dy = table("baja_band_dy")
    half = table("baja_band_half_width")
    if len(dy) != len(half) + 1:
        raise SystemExit("band tables disagree")
    if dy[-1] != SCREEN_H - HORIZON_Y:
        raise SystemExit("band table does not reach the bottom scanline")
    return dy, half


def load_rgba(name: str) -> np.ndarray:
    return np.array(Image.open(RAW / name).convert("RGBA")).astype(np.int32)


def hard_alpha(rgba: np.ndarray, threshold: int = 128) -> np.ndarray:
    out = rgba.copy()
    solid = out[..., 3] >= threshold
    out[..., 3] = np.where(solid, 255, 0)
    out[..., :3] = np.where(solid[..., None], out[..., :3], 0)
    return out


def box_resize(rgba: np.ndarray, width: int, height: int) -> np.ndarray:
    """Area-average resize that keeps alpha binary.

    Premultiplying first stops transparent black bleeding into the edges of a
    shrunk sprite, which is what produces the halo seams the vision rejects.
    """
    src = Image.fromarray(rgba.astype(np.uint8), "RGBA")
    alpha = np.asarray(src.getchannel("A")).astype(np.float64) / 255.0
    rgb = np.asarray(src.convert("RGB")).astype(np.float64)
    premultiplied = np.dstack([rgb * alpha[..., None], alpha * 255.0])
    small = np.asarray(
        Image.fromarray(premultiplied.astype(np.uint8), "RGBA").resize(
            (width, height), Image.BOX)).astype(np.float64)
    a = small[..., 3:4] / 255.0
    with np.errstate(divide="ignore", invalid="ignore"):
        colour = np.where(a > 0.0, small[..., :3] / np.maximum(a, 1e-6), 0.0)
    out = np.dstack([np.clip(colour, 0, 255), small[..., 3]])
    solid = out[..., 3] >= 128
    out[..., 3] = np.where(solid, 255, 0)
    out[..., :3] = np.where(solid[..., None], out[..., :3], 0)
    return out.astype(np.int32)


def neo_quantise(value: np.ndarray) -> np.ndarray:
    """Snap to the Neo Geo 5-bit-per-channel hardware grid."""
    q = np.clip((value.astype(np.int32) * 31 + 127) // 255, 0, 31)
    return (q * 255 + 15) // 31


def pick_palette(rgba: np.ndarray, count: int = 15, weight: np.ndarray | None = None) -> list[str]:
    """Choose `count` opaque colours with a deterministic weighted k-means.

    Median cut on the hardware grid loses the narrow value ramps this art
    relies on, so the palette is fitted to the actual pixel distribution.
    """
    solid = rgba[..., 3] >= 128
    pixels = neo_quantise(rgba[..., :3][solid]).astype(np.float64)
    if pixels.size == 0:
        return ["#000000"]
    w = np.ones(len(pixels)) if weight is None else weight[solid].astype(np.float64)
    unique, inverse = np.unique(pixels, axis=0, return_inverse=True)
    counts = np.bincount(inverse, weights=w)
    if len(unique) <= count:
        order = np.argsort(-counts)
        return ["#%02X%02X%02X" % tuple(int(c) for c in unique[i]) for i in order]

    # Deterministic seeding: most common colour, then farthest-point.
    centres = [unique[int(np.argmax(counts))]]
    distance = np.sum((unique - centres[0]) ** 2, axis=1)
    while len(centres) < count:
        centres.append(unique[int(np.argmax(distance * counts))])
        distance = np.minimum(distance, np.sum((unique - centres[-1]) ** 2, axis=1))
    centres = np.array(centres, dtype=np.float64)

    for _ in range(24):
        d = ((unique[:, None, :] - centres[None, :, :]) ** 2).sum(axis=2)
        owner = np.argmin(d, axis=1)
        moved = 0.0
        for k in range(count):
            mask = owner == k
            if not mask.any():
                continue
            new = (unique[mask] * counts[mask, None]).sum(axis=0) / counts[mask].sum()
            moved += float(np.abs(new - centres[k]).sum())
            centres[k] = new
        if moved < 1.0:
            break
    centres = neo_quantise(np.round(centres))
    seen: list[tuple[int, int, int]] = []
    for centre in centres:
        key = tuple(int(v) for v in centre)
        if key not in seen:
            seen.append(key)
    return ["#%02X%02X%02X" % c for c in seen]


def neo_word(rgb: tuple[int, int, int]) -> int:
    """Pack an 8-bit colour into the Neo Geo palette word, as the SDK compiler does."""
    r5 = (rgb[0] * 31 + 127) // 255
    g5 = (rgb[1] * 31 + 127) // 255
    b5 = (rgb[2] * 31 + 127) // 255
    return (((r5 & 1) << 14) | ((g5 & 1) << 13) | ((b5 & 1) << 12)
            | ((r5 >> 1) << 8) | ((g5 >> 1) << 4) | (b5 >> 1))


def hex_rgb(colour: str) -> tuple[int, int, int]:
    return tuple(int(colour[i:i + 2], 16) for i in (1, 3, 5))  # type: ignore[return-value]


def save_sheet(rgba: np.ndarray, name: str) -> Path:
    SOURCE.mkdir(parents=True, exist_ok=True)
    path = SOURCE / name
    Image.fromarray(rgba.astype(np.uint8), "RGBA").save(path)
    return path


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
