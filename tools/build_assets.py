#!/usr/bin/env python3
"""Convert the approved BAJANEW artwork into Forge68 asset sources.

Inputs are only the approved originals under art/ plus the hand-authored FIX
glyphs.  Every sheet is pre-quantised to the palette it declares, so the ROM
compiler performs an exact index lookup and the conversion is reproducible.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import fixfont
import road_strips
from bajaart import (ASSETS, FUNNEL_K, HORIZON_Y, RAW, ROOT, SOURCE, TEX_U_SPAN,
                     TEX_W, band_tables, box_resize, hard_alpha, hex_rgb, load_rgba,
                     neo_quantise, neo_word, pick_palette, save_sheet, sha256,
                     write_json)

SPLASH_SOURCE = ROOT / "02_REFERENCE_LIBRARY/developer-splash/devsplashlogo.jpg"

# ---------------------------------------------------------------- geometry --

# The frame of every vehicle sheet spans this much world width, so one shared
# projection sizes the player and the rivals identically.
VEHICLE_FRAME_M = 2.6
PLAYER_FRAME = (112, 96)
RIVAL_LODS = [(96, 96), (64, 64), (32, 32), (16, 16)]

SCREEN_W, SCREEN_H = 320, 224
# Backdrop layers.  Both are cut from one scaling of the plate so the join at
# the horizon is seamless: the sky layer pans a little with the bends and the
# ground layer, being nearer, pans more.
BACKDROP_SCALE = 540.0 / 1498.0
# The sky stops a few rows above the horizon and the ground layer, cut from
# the same rows of the plate, takes over there: the join is invisible and the
# sky's hardware footprint ends where its pixels do.
GROUND_ABOVE_HORIZON = 4
SKY_H = HORIZON_Y - GROUND_ABOVE_HORIZON
GROUND_H = 112
SKY_PAN = 40
GROUND_PAN = 60
# Palette slots: scenery kinds from 40, dust at 60, one per road band from 64.
SCENERY_PALETTE_BASE = 40
DUST_PALETTE = 60
ROAD_PALETTE_BASE = 64
ROAD_HAZE = (214, 196, 170)

PLAYER_BOXES = [(6, 169, 383, 586), (420, 191, 840, 592), (877, 255, 1283, 586),
                (1317, 176, 1725, 592), (1755, 70, 2143, 587)]
# Sheet order the game indexes: neutral, left, right, air, settled.
PLAYER_ORDER = [0, 3, 1, 4, 2]

RIVAL_BOXES = [(1423, 98, 2042, 665), (838, 114, 1374, 623),
               (373, 211, 766, 588), (31, 322, 297, 564)]

PROPS_COLUMNS = [(29, 294), (342, 619), (657, 938), (997, 1245)]
PROPS_ROWS = [(35, 279), (355, 571), (601, 881), (941, 1189)]

# name, (props column, props row), frame size, world width in metres
SCENERY = [
    ("rock_pale", (0, 1), (48, 32), 3.0),
    ("rock_grey", (1, 1), (48, 32), 3.2),
    ("agave", (2, 1), (48, 48), 2.6),
    ("bush", (3, 1), (48, 32), 2.6),
    ("palm", (0, 2), (64, 96), 7.0),
    ("cactus", (1, 2), (48, 80), 4.0),
    ("chevron", (2, 2), (48, 48), 2.2),
    ("flag", (3, 2), (32, 64), 1.8),
    ("crowd", (0, 3), (64, 48), 4.2),
]
# Signs are the chevron sign's posts and board with the board repainted and
# lettered in the game's own typeface; gantries are the same board stretched
# across the road on two of its posts.  name, lines, frame, world width.
SIGNS = [
    ("sign_ensenada", ("ENSENADA",), (64, 48), 3.6),
    ("sign_pacific", ("PACIFIC", "RUN"), (64, 48), 3.6),
    ("sign_baja", ("BAJA", "1000"), (64, 48), 3.6),
    ("gantry_start", ("START",), (144, 64), 10.0),
    ("gantry_finish", ("FINISH",), (144, 64), 10.0),
]
BOARD_FACE = (242, 230, 200)
BOARD_EDGE = (70, 40, 20)
BOARD_INK = (18, 20, 31)
BOARD_ACCENT = (200, 40, 30)
DUST_CELLS = [(1, 3), (2, 3)]
DUST_FRAME = (32, 32)
DUST_WORLD_M = 3.6

FIX_PALETTE = ["#F2EDDF", "#FFC132", "#63DCEF", "#FF5B45", "#7BDF86", "#12141F"]

record: dict = {"sheets": [], "sources": {}}


def note_source(name: str) -> None:
    record["sources"].setdefault(name, sha256(RAW / name))


# ------------------------------------------------------------- quantisation --

def quantise_to(rgba: np.ndarray, palette: list[str]) -> np.ndarray:
    """Snap every opaque pixel to the declared palette."""
    colours = np.array([[int(c[i:i + 2], 16) for i in (1, 3, 5)] for c in palette],
                       dtype=np.int32)
    solid = rgba[..., 3] >= 128
    pixels = neo_quantise(rgba[..., :3])
    distance = ((pixels[..., None, :] - colours[None, None, :, :]) ** 2).sum(axis=3)
    nearest = colours[np.argmin(distance, axis=2)]
    out = rgba.copy()
    out[..., :3] = np.where(solid[..., None], nearest, 0)
    out[..., 3] = np.where(solid, 255, 0)
    return out


def emit(name: str, rgba: np.ndarray, frame: tuple[int, int], origin: tuple[int, int],
         palette_index: int, palette: list[str], extra: dict | None = None) -> dict:
    rgba = quantise_to(hard_alpha(rgba), palette)
    path = save_sheet(rgba, f"{name}.png")
    entry = {
        "name": name,
        "source": f"source/{name}.png",
        "frame_width": frame[0],
        "frame_height": frame[1],
        "origin": [origin[0], origin[1]],
        "palette": palette_index,
        "palette_colors": palette,
        "clips": [{"name": "still", "frames": [0], "durations": [1], "loop": True}],
    }
    frames = (rgba.shape[1] // frame[0]) * (rgba.shape[0] // frame[1])
    if frames > 1:
        entry["clips"] = [{
            "name": "all",
            "frames": list(range(frames)),
            "durations": [8] * frames,
            "loop": True,
        }]
    summary = {"name": name, "frames": frames, "sheet": list(rgba.shape[1::-1]),
               "frame": list(frame), "palette": palette_index,
               "sha256": sha256(path)}
    if extra:
        summary.update(extra)
    record["sheets"].append(summary)
    return entry


def fit_sprite(rgba: np.ndarray, frame: tuple[int, int], pad: float = 0.98,
               anchor: str = "bottom") -> np.ndarray:
    """Place a cut-out inside a frame, bottom-anchored and centred."""
    solid = rgba[..., 3] >= 128
    ys, xs = np.where(solid)
    if len(xs) == 0:
        return np.zeros((frame[1], frame[0], 4), dtype=np.int32)
    cut = rgba[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    scale = min(frame[0] * pad / cut.shape[1], frame[1] * pad / cut.shape[0])
    width = max(1, int(round(cut.shape[1] * scale)))
    height = max(1, int(round(cut.shape[0] * scale)))
    small = box_resize(cut, width, height)
    out = np.zeros((frame[1], frame[0], 4), dtype=np.int32)
    x0 = (frame[0] - width) // 2
    y0 = frame[1] - height if anchor == "bottom" else (frame[1] - height) // 2
    out[y0:y0 + height, x0:x0 + width] = small
    return out


# -------------------------------------------------------------------- road --

def shade(colours: list[tuple[int, int, int]], gain: float) -> list[tuple[int, int, int]]:
    return [tuple(int(round(min(255.0, c * gain))) for c in rgb) for rgb in colours]  # type: ignore[misc]


def haze(colours: list[tuple[int, int, int]], depth_m: float) -> list[tuple[int, int, int]]:
    """Aerial perspective: the far road fades toward the dust in the air."""
    t = min(0.5, max(0.0, (depth_m - 30.0) / 260.0))
    return [tuple(int(round(c * (1.0 - t) + h * t)) for c, h in zip(rgb, ROAD_HAZE))  # type: ignore[misc]
            for rgb in colours]


def palette_words(colours: list[tuple[int, int, int]]) -> list[int]:
    words = [0x8000] + [neo_word(neo_quantise(np.array(c)).tolist()) for c in colours]  # type: ignore[arg-type]
    while len(words) < 16:
        words.append(0x8000)
    return words


def build_road(sprites: list[dict]) -> dict:
    strips, report = road_strips.build_sheets()
    note_source(road_strips.PLATE)
    palettes = []
    total_columns = 0
    for name, strip, verge, geometry in strips:
        solid = hard_alpha(strip)
        # Road surface and verge get their own entries so the phase can darken
        # the verge hard while the surface only breathes.
        road_pixels = solid.copy()
        road_pixels[verge, 3] = 0
        verge_pixels = solid.copy()
        verge_pixels[~verge, 3] = 0
        road_palette = pick_palette(road_pixels, 9)
        verge_palette = pick_palette(verge_pixels, 6) if verge.any() else []
        palette = road_palette + [c for c in verge_palette if c not in road_palette]
        while len(palette) < 15:
            palette.append("#%02X%02X%02X" % (len(palette) * 3, 0, 0))
        palette = palette[:15]
        base = [hex_rgb(c) for c in palette]
        is_verge = [c not in road_palette for c in palette]
        depth = geometry["depth_mid_m"]
        phase_a = haze(base, depth)
        phase_b = haze([shade([c], 0.80 if v else 0.96)[0] for c, v in zip(base, is_verge)], depth)
        palettes.append({"band": geometry["band"],
                         "phase_a": palette_words(phase_a),
                         "phase_b": palette_words(phase_b)})
        sprites.append(emit(name, solid, (geometry["width"], geometry["height"]),
                            (geometry["width"] // 2, 0),
                            ROAD_PALETTE_BASE + geometry["band"], palette,
                            {"band": geometry["band"], "phases": geometry["phases"],
                             "columns_on_screen": geometry["columns_on_screen"],
                             "strip_columns": geometry["tiles_x"],
                             "tile_rows": geometry["tiles_y"]}))
        total_columns += geometry["columns_on_screen"]
    report["on_screen_columns"] = total_columns
    report["palettes"] = palettes
    report["palette_base"] = ROAD_PALETTE_BASE
    report["haze"] = list(ROAD_HAZE)
    report["phase_gain"] = {"road": 0.96, "verge": 0.80}
    return report


def build_splash(sprites: list[dict]) -> dict:
    """The user-supplied developer mark, letterboxed at native resolution.

    Aspect ratio is preserved and the source stays byte-for-byte untouched; the
    matte is the logo's own background colour so the plate reads as one image.
    """
    logo = np.array(Image.open(SPLASH_SOURCE).convert("RGBA")).astype(np.int32)
    height = SCREEN_H
    width = int(round(logo.shape[1] * height / logo.shape[0]))
    small = box_resize(logo, width, height)
    matte = np.zeros((SCREEN_H, SCREEN_W, 4), dtype=np.int32)
    corner = small[2, 2, :3]
    matte[..., :3] = corner
    matte[..., 3] = 255
    x0 = (SCREEN_W - width) // 2
    matte[:, x0:x0 + width] = small
    palette = pick_palette(matte)
    sprites.append(emit("splash", matte, (SCREEN_W, SCREEN_H), (0, 0), 17, palette))
    return {"source": str(SPLASH_SOURCE.relative_to(ROOT)),
            "source_sha256": sha256(SPLASH_SOURCE),
            "letterbox": [x0, 0, width, height], "aspect_preserved": True}


DRIVER_SOURCE = "max-cruz-select-v2.png"
DRIVER_BOXES = [(30, 31, 738, 990), (840, 151, 1492, 990)]
DRIVER_FRAME = (96, 128)


def build_drivers(sprites: list[dict]) -> dict:
    """Max and Cruz for the selection screen.

    The source ships its transparency as a near-white matte, so it is keyed out
    rather than trusted as alpha.
    """
    note_source(DRIVER_SOURCE)
    plate = np.array(Image.open(RAW / DRIVER_SOURCE).convert("RGB")).astype(np.int32)
    low, high = plate.min(axis=2), plate.max(axis=2)
    matte = (low >= 235) & ((high - low) <= 14)
    rgba = np.dstack([plate, np.where(matte, 0, 255).astype(np.int32)])

    out = []
    for index, (name, box) in enumerate(zip(("driver_max", "driver_cruz"), DRIVER_BOXES)):
        x0, y0, x1, y1 = box
        art = fit_sprite(rgba[y0:y1 + 1, x0:x1 + 1], DRIVER_FRAME)
        palette = pick_palette(art)
        sprites.append(emit(name, art, DRIVER_FRAME,
                            (DRIVER_FRAME[0] // 2, DRIVER_FRAME[1] - 1),
                            18 + index, palette))
        out.append(name)
    return {"source": DRIVER_SOURCE, "frame": list(DRIVER_FRAME), "drivers": out}


# ---------------------------------------------------------------- backdrop --

def build_backdrop(sprites: list[dict]) -> dict:
    """Sky and ground layers from one scaling of the Ensenada plate.

    The plate is composed with its road vanishing at PLATE_VX, PLATE_VY.  Scaled
    so its vanishing point lands on the horizon at the neutral pan, its top
    becomes the sky layer (sea, cliffs, mountains, clouds) and the band below
    the horizon becomes the ground layer, with the plate's own road painted
    out by mirroring the hillside inward so no second road ever shows behind
    the real bands.  Neither layer is mirrored end to end: each is simply wide
    enough for its pan.
    """
    note_source(road_strips.PLATE)
    plate = load_rgba(road_strips.PLATE)
    width = int(round(plate.shape[1] * BACKDROP_SCALE))
    height = int(round(plate.shape[0] * BACKDROP_SCALE))
    fitted = box_resize(plate, width, height).astype(np.float64)
    vp_x = road_strips.PLATE_VX * BACKDROP_SCALE
    vp_y = int(round(road_strips.PLATE_VY * BACKDROP_SCALE))

    sky_top = vp_y - HORIZON_Y
    sky = fitted[sky_top:sky_top + SKY_H].copy()
    sky[..., 3] = 255.0

    ground = fitted[vp_y - GROUND_ABOVE_HORIZON:vp_y - GROUND_ABOVE_HORIZON + GROUND_H].copy()
    plate_slope = road_strips.PLATE_K
    for row in range(GROUND_ABOVE_HORIZON, GROUND_H):
        source = ground[row].copy()
        half = plate_slope * float(row - GROUND_ABOVE_HORIZON) + 2.0
        left_edge = int(round(vp_x - half))
        right_edge = int(round(vp_x + half))
        for x in range(max(0, left_edge), min(width, right_edge + 1)):
            mirrored = 2 * left_edge - x if x <= vp_x else 2 * right_edge - x
            ground[row, x] = source[int(np.clip(mirrored, 0, width - 1))]
    ground[..., 3] = 255.0

    # Where the neutral window starts on each strip: the vanishing point sits
    # at the screen centre when the road runs straight.
    sky_origin = int(round(vp_x)) - SCREEN_W // 2
    strip_w = (width // 16) * 16
    sky_palette = pick_palette(sky.astype(np.int32))
    sprites.append(emit("sky", sky[:, :strip_w].astype(np.int32), (strip_w, SKY_H), (0, 0), 1, sky_palette))
    ground_palette = pick_palette(ground.astype(np.int32))
    sprites.append(emit("ground", ground[:, :strip_w].astype(np.int32), (strip_w, GROUND_H), (0, 0), 2,
                        ground_palette))
    if sky_origin - SKY_PAN < 0 or sky_origin + SCREEN_W + GROUND_PAN > strip_w:
        raise SystemExit("backdrop strip is too narrow for its pan")
    return {"scale": BACKDROP_SCALE, "strip_width": strip_w, "sky_height": SKY_H,
            "ground_height": GROUND_H, "origin_x": sky_origin,
            "ground_y": HORIZON_Y - GROUND_ABOVE_HORIZON,
            "sky_pan": SKY_PAN, "ground_pan": GROUND_PAN, "horizon_row": HORIZON_Y}


# ------------------------------------------------------------------ actors --

def build_player(sprites: list[dict]) -> dict:
    note_source("player-xr22-sheet.png")
    sheet_src = load_rgba("player-xr22-sheet.png")
    frames = []
    for index in PLAYER_ORDER:
        x0, y0, x1, y1 = PLAYER_BOXES[index]
        frames.append(fit_sprite(sheet_src[y0:y1 + 1, x0:x1 + 1], PLAYER_FRAME))
    sheet = np.concatenate(frames, axis=1)
    palette = pick_palette(sheet)
    sprites.append(emit("player", sheet, PLAYER_FRAME,
                        (PLAYER_FRAME[0] // 2, PLAYER_FRAME[1] - 2), 4, palette,
                        {"poses": ["neutral", "left", "right", "air", "settled"]}))
    return {"frame_world_metres": VEHICLE_FRAME_M, "source_order": PLAYER_ORDER}


def build_rivals(sprites: list[dict]) -> dict:
    note_source("rival-orange-sheet.png")
    plate = np.array(Image.open(RAW / "rival-orange-sheet.png").convert("RGB")).astype(np.int32)
    low, high = plate.min(axis=2), plate.max(axis=2)
    # The sheet ships its transparency as a light checkerboard; key it out.
    background = (low >= 228) & ((high - low) <= 16)
    rgba = np.dstack([plate, np.where(background, 0, 255).astype(np.int32)])

    cuts = []
    for x0, y0, x1, y1 in RIVAL_BOXES:
        cuts.append(rgba[y0:y1 + 1, x0:x1 + 1])

    lods = []
    for index, frame in enumerate(RIVAL_LODS):
        lods.append(fit_sprite(cuts[min(index, len(cuts) - 1)], frame))

    base = pick_palette(np.concatenate([l.reshape(1, -1, 4) for l in lods], axis=1))
    # The second entry is the same field car under a different livery, which is
    # a palette swap on hardware and costs no extra tiles.
    alternate = []
    for colour in base:
        r, g, b = (int(colour[i:i + 2], 16) for i in (1, 3, 5))
        alternate.append("#%02X%02X%02X" % (
            min(255, int(b * 0.90 + 18)), min(255, int(r * 0.82 + 30)),
            min(255, int(g * 0.86 + 24))))
    seen: list[str] = []
    for colour in alternate:
        if colour not in seen:
            seen.append(colour)
    while len(seen) < len(base):
        seen.append("#%02X%02X%02X" % (len(seen), len(seen) * 2, len(seen) * 3))

    for index, (frame, art) in enumerate(zip(RIVAL_LODS, lods)):
        sprites.append(emit(f"rival_a{index}", art, frame,
                            (frame[0] // 2, frame[1] - 1), 5, base,
                            {"lod": index}))
    for index, (frame, art) in enumerate(zip(RIVAL_LODS, lods)):
        # Same pixels, second palette: the compiler dedups the tiles.
        art_copy = quantise_to(hard_alpha(art), base)
        remap = {c: seen[i] for i, c in enumerate(base)}
        swapped = art_copy.copy()
        for source_colour, target_colour in remap.items():
            src = np.array([int(source_colour[i:i + 2], 16) for i in (1, 3, 5)])
            dst = np.array([int(target_colour[i:i + 2], 16) for i in (1, 3, 5)])
            mask = np.all(art_copy[..., :3] == src, axis=2) & (art_copy[..., 3] > 0)
            swapped[mask, :3] = dst
        sprites.append(emit(f"rival_b{index}", swapped, frame,
                            (frame[0] // 2, frame[1] - 1), 6, seen, {"lod": index}))
    return {"frame_world_metres": VEHICLE_FRAME_M,
            "lods": [list(f) for f in RIVAL_LODS]}


def letter(canvas: np.ndarray, text: str, x: int, y: int, scale: int,
           ink: tuple[int, int, int]) -> None:
    """Blit the HUD typeface onto a sprite, seven columns per glyph."""
    for index, ch in enumerate(text):
        rows = fixfont.GLYPHS.get(ch, fixfont.GLYPHS[" "])
        for gy, row in enumerate(rows):
            for gx, cell in enumerate(row[:7]):
                if cell != "#":
                    continue
                x0 = x + (index * 7 + gx) * scale
                y0 = y + gy * scale
                canvas[y0:y0 + scale, x0:x0 + scale, :3] = ink
                canvas[y0:y0 + scale, x0:x0 + scale, 3] = 255


def build_sign(props: np.ndarray, lines: tuple[str, ...], frame: tuple[int, int]) -> np.ndarray:
    """A lettered sign from the approved chevron sign's posts and board.

    The chevron's black board is found by its darkness, repainted as a cream
    face with a wooden edge, and lettered.  A gantry keeps two copies of the
    posts at the frame's edges and stretches the board between them.
    """
    x0, x1 = PROPS_COLUMNS[2]
    y0, y1 = PROPS_ROWS[2]
    chevron = props[y0:y1 + 1, x0:x1 + 1]
    gantry = frame[0] > 96
    base = fit_sprite(chevron, (64, 48) if not gantry else (48, 64))
    solid = base[..., 3] >= 128
    dark = solid & (base[..., :3].max(axis=2) < 70)
    ys, xs = np.where(dark[: int(base.shape[0] * 0.62)])
    by0, by1, bx0, bx1 = ys.min(), ys.max(), xs.min(), xs.max()
    out = np.zeros((frame[1], frame[0], 4), dtype=np.int32)
    if not gantry:
        out[:] = base
        # The board keeps the sign's outline and gains a face and an edge.
        board = np.zeros_like(out[by0:by1 + 1, bx0:bx1 + 1])
        board[..., :3] = BOARD_EDGE
        board[..., 3] = 255
        board[1:-1, 1:-1, :3] = BOARD_FACE
        out[by0:by1 + 1, bx0:bx1 + 1] = board
        face_w, face_h = bx1 - bx0 - 1, by1 - by0 - 1
        scale = 1
        text_h = len(lines) * 8 * scale + (len(lines) - 1) * scale
        ty = by0 + 1 + max(0, (face_h - text_h) // 2)
        for line in lines:
            tx = bx0 + 1 + max(0, (face_w - len(line) * 7 * scale) // 2)
            letter(out, line, tx, ty, scale, BOARD_INK)
            ty += 9 * scale
        return out
    # Gantry: posts at each side, a banner across the top.
    post = base[:, bx0:bx1 + 1].copy()
    post[: by1 + 1] = 0  # keep the legs only
    legs = post[by1 + 1:]
    leg_h = legs.shape[0]
    leg_w = legs.shape[1]
    scale_h = (frame[1] - 22) / leg_h
    legs = box_resize(legs, max(6, int(leg_w * scale_h * 0.35)), frame[1] - 22)
    out[22:, 2:2 + legs.shape[1]] = legs
    out[22:, frame[0] - 2 - legs.shape[1]:frame[0] - 2] = legs
    banner = np.zeros((24, frame[0], 4), dtype=np.int32)
    banner[..., :3] = BOARD_EDGE
    banner[..., 3] = 255
    banner[2:-2, 2:-2, :3] = BOARD_FACE
    banner[2:4, 2:-2, :3] = BOARD_ACCENT
    banner[-4:-2, 2:-2, :3] = BOARD_ACCENT
    text = lines[0]
    tx = (frame[0] - len(text) * 14) // 2
    letter(banner, text, tx, 4, 2, BOARD_INK)
    out[2:26] = banner
    return out


def far_frame(frame: tuple[int, int]) -> tuple[int, int]:
    """One column wide, keeping the prop's proportions in tile rows."""
    height = max(16, min(48, (frame[1] * 16 // frame[0] + 15) // 16 * 16))
    return (16, height)


def emit_prop(sprites: list[dict], name: str, art: np.ndarray, frame: tuple[int, int],
              palette_index: int, world: float, extra: dict) -> None:
    """A prop and its far frame, sharing one palette so both quantise alike."""
    palette = pick_palette(art)
    sprites.append(emit(name, art, frame, (frame[0] // 2, frame[1] - 1),
                        palette_index, palette, dict(extra, world_metres=world)))
    far = far_frame(frame)
    small = fit_sprite(art, far, pad=1.0)
    sprites.append(emit(f"{name}_far", small, far, (far[0] // 2, far[1] - 1),
                        palette_index, palette, dict(extra, world_metres=world, far=True)))


def build_scenery(sprites: list[dict]) -> dict:
    note_source("ensenada-props.png")
    props = load_rgba("ensenada-props.png")
    out = []
    for index, (name, (col, row), frame, world) in enumerate(SCENERY):
        x0, x1 = PROPS_COLUMNS[col]
        y0, y1 = PROPS_ROWS[row]
        art = fit_sprite(props[y0:y1 + 1, x0:x1 + 1], frame)
        emit_prop(sprites, name, art, frame, SCENERY_PALETTE_BASE + index, world, {})
        out.append({"name": name, "frame": list(frame), "world_metres": world})
    for index, (name, lines, frame, world) in enumerate(SIGNS):
        art = build_sign(props, lines, frame)
        emit_prop(sprites, name, art, frame, SCENERY_PALETTE_BASE + len(SCENERY) + index,
                  world, {"text": list(lines)})
        out.append({"name": name, "frame": list(frame), "world_metres": world,
                    "text": list(lines)})

    dust_frames = []
    for col, row in DUST_CELLS:
        x0, x1 = PROPS_COLUMNS[col]
        y0, y1 = PROPS_ROWS[row]
        dust_frames.append(fit_sprite(props[y0:y1 + 1, x0:x1 + 1], DUST_FRAME))
    # A third step fades the puff out so the trail has a tail, not a pop.
    fade = dust_frames[1].copy()
    fade[..., 3] = np.where(
        (fade[..., 3] > 0) & (((np.arange(DUST_FRAME[1])[:, None]
                                + np.arange(DUST_FRAME[0])[None, :]) % 2) == 0), 255, 0)
    dust_frames.append(fade)
    dust = np.concatenate(dust_frames, axis=1)
    dust_palette = pick_palette(dust)
    sprites.append(emit("dust", dust, DUST_FRAME, (DUST_FRAME[0] // 2, DUST_FRAME[1] - 4),
                        DUST_PALETTE, dust_palette, {"world_metres": DUST_WORLD_M}))
    return {"scenery": out, "dust_world_metres": DUST_WORLD_M}


# --------------------------------------------------------------------- FIX --

def build_fix() -> dict:
    sheet = fixfont.build_sheet()
    SOURCE.mkdir(parents=True, exist_ok=True)
    path = SOURCE / "font.png"
    Image.fromarray(sheet.astype(np.uint8), "RGBA").save(path)
    return {"glyphs": len(fixfont.GLYPHS) + len(fixfont.BLOCKS),
            "sha256": sha256(path), "shades": ["ivory", "amber", "cyan"]}


# -------------------------------------------------------------------- main --

def c_identifier(name: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in name)


def emit_c_tables(road: dict, backdrop: dict, scenery: dict, out_dir: Path) -> None:
    """Emit the frame tables the renderer indexes, so art and code cannot drift."""
    bands = road_strips.strip_geometry()
    lines = [
        "/* Generated by tools/build_assets.py.  Do not edit. */",
        '#include "game/bajanew_assets.h"',
        "",
        "const BajanewStripDef bajanew_road_strip[BAJA_ROAD_BANDS] = {",
    ]
    for geometry in bands:
        name = f"road{geometry['band']:02d}"
        lines.append(f"    {{&ng_asset_{name}_frames[0], {geometry['tiles_x']}, "
                     f"{geometry['columns_on_screen']}, {geometry['tiles_y']}, "
                     f"{ROAD_PALETTE_BASE + geometry['band']}, {geometry['height']}}},")
    lines[-1] = lines[-1].rstrip(",")
    lines += ["};", "",
              "const uint16_t bajanew_road_palette[BAJA_ROAD_BANDS][2][16] = {"]
    for entry in road["palettes"]:
        a = ", ".join(f"0x{w:04x}" for w in entry["phase_a"])
        b = ", ".join(f"0x{w:04x}" for w in entry["phase_b"])
        lines.append(f"    {{{{{a}}},")
        lines.append(f"     {{{b}}}}},")
    lines[-1] = lines[-1].rstrip(",")
    lines += ["};", "",
              f"const int16_t bajanew_backdrop_origin_x = {backdrop['origin_x']};",
              f"const int16_t bajanew_ground_y = {backdrop['ground_y']};",
              f"const int16_t bajanew_sky_pan = {backdrop['sky_pan']};",
              f"const int16_t bajanew_ground_pan = {backdrop['ground_pan']};",
              "",
              "const BajanewSpriteDef bajanew_scenery[BAJA_SCENERY_KINDS] = {"]
    for name, _cell, _frame, world in SCENERY:
        lines.append(f"    {{&ng_asset_{c_identifier(name)}_frames[0], {int(round(world * 256))}}},")
    for name, _text, _frame, world in SIGNS:
        lines.append(f"    {{&ng_asset_{c_identifier(name)}_frames[0], {int(round(world * 256))}}},")
    lines[-1] = lines[-1].rstrip(",")
    lines += ["};", "", "const BajanewSpriteDef bajanew_scenery_far[BAJA_SCENERY_KINDS] = {"]
    for name, _cell, _frame, world in SCENERY:
        lines.append(f"    {{&ng_asset_{c_identifier(name)}_far_frames[0], {int(round(world * 256))}}},")
    for name, _text, _frame, world in SIGNS:
        lines.append(f"    {{&ng_asset_{c_identifier(name)}_far_frames[0], {int(round(world * 256))}}},")
    lines[-1] = lines[-1].rstrip(",")
    lines += ["};", "",
              "const BajanewSpriteDef bajanew_rival[2][BAJANEW_RIVAL_LODS] = {"]
    for side in ("a", "b"):
        entries = ", ".join(
            f"{{&ng_asset_rival_{side}{i}_frames[0], {int(round(VEHICLE_FRAME_M * 256))}}}"
            for i in range(len(RIVAL_LODS)))
        lines.append(f"    {{{entries}}},")
    lines[-1] = lines[-1].rstrip(",")
    lines += ["};", "",
              "const uint8_t bajanew_rival_lod_width[BAJANEW_RIVAL_LODS] = {"]
    lines.append("    " + ", ".join(str(f[0]) for f in RIVAL_LODS))
    lines += ["};", "",
              f"const BajanewSpriteDef bajanew_player_def = "
              f"{{&ng_asset_player_frames[0], {int(round(VEHICLE_FRAME_M * 256))}}};",
              f"const BajanewSpriteDef bajanew_dust_def = "
              f"{{&ng_asset_dust_frames[0], {int(round(DUST_WORLD_M * 256))}}};",
              ""]
    (out_dir / "bajanew_assets.c").write_text("\n".join(lines), encoding="utf-8")
    # RAM the strip layers need for their prebuilt tile map words.
    words = sum(g["tiles_x"] * g["tiles_y"] * 2 for g in bands)
    words += backdrop["strip_width"] // 16 * (backdrop["sky_height"] // 16) * 2
    words += backdrop["strip_width"] // 16 * (backdrop["ground_height"] // 16) * 2
    road_slots = 21 + 21 + sum(g["columns_on_screen"] for g in bands)
    (out_dir / "bajanew_assets_config.h").write_text(
        "/* Generated by tools/build_assets.py.  Do not edit. */\n"
        "#ifndef BAJANEW_ASSETS_CONFIG_H\n#define BAJANEW_ASSETS_CONFIG_H\n"
        f"#define BAJANEW_STRIP_WORDS {words}\n"
        "/* Hardware sprites the sky, ground and road bands own; objects use the rest. */\n"
        f"#define BAJANEW_STRIP_SLOTS {road_slots}\n#endif\n", encoding="utf-8")


def main() -> None:
    sprites: list[dict] = []
    road = build_road(sprites)
    backdrop = build_backdrop(sprites)
    splash = build_splash(sprites)
    drivers = build_drivers(sprites)
    player = build_player(sprites)
    rivals = build_rivals(sprites)
    scenery = build_scenery(sprites)
    fix = build_fix()

    manifest = {
        "format": 2,
        "art_policy": {
            "mode": "bajanew_approved_originals",
            "note": "The approved raws under art/ and the hand-authored FIX "
                    "glyphs are the shipping sources; any new art is generated "
                    "with Greg's Grok Build account and retained raw under art/.",
            "native_resolution": "320x224",
            "resampling": "area_average_downscale_then_exact_palette_index",
        },
        "c_rom_bytes_per_chip": 1048576,
        "s_rom_bytes": 131072,
        "fix_source": "source/font.png",
        "fix_palette": FIX_PALETTE,
        "sprites": sprites,
    }
    write_json(ASSETS / "manifest.json", manifest)
    generated = ROOT / "build/assets/generated"
    generated.mkdir(parents=True, exist_ok=True)
    emit_c_tables(road, backdrop, scenery, generated)
    write_json(ROOT / "build/assets/CONVERSION.json", {
        "road": road, "backdrop": backdrop, "splash": splash, "drivers": drivers,
        "player": player, "rivals": rivals,
        "scenery": scenery, "fix": fix,
        "sources": record["sources"], "sheets": record["sheets"],
    })
    columns = road["on_screen_columns"]
    print(json.dumps({"sprites": len(sprites),
                      "road_columns_on_screen": columns,
                      "fix_glyphs": fix["glyphs"]}, sort_keys=True))


if __name__ == "__main__":
    main()
