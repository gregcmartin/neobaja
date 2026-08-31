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
                     TEX_W, band_tables, box_resize, hard_alpha, load_rgba,
                     neo_quantise, pick_palette, save_sheet, sha256, write_json)

SPLASH_SOURCE = ROOT / "02_REFERENCE_LIBRARY/developer-splash/devsplashlogo.jpg"

# ---------------------------------------------------------------- geometry --

# The frame of every vehicle sheet spans this much world width, so one shared
# projection sizes the player and the rivals identically.
VEHICLE_FRAME_M = 2.6
PLAYER_FRAME = (112, 96)
RIVAL_LODS = [(96, 96), (64, 64), (32, 32), (16, 16)]

SCREEN_W, SCREEN_H = 320, 224
BACKDROP_W = 640
BACKDROP_H = 176

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

def build_road(sprites: list[dict]) -> dict:
    strips, report = road_strips.build_sheets()
    note_source(road_strips.PLATE)

    stack = np.concatenate([s.reshape(-1, 4) for _, s, _ in strips], axis=0)
    palette = pick_palette(stack.reshape(1, -1, 4))
    while len(palette) < 15:
        palette.append("#%02X%02X%02X" % (len(palette) * 3, 0, 0))
    colours = np.array([[int(c[i:i + 2], 16) for i in (1, 3, 5)] for c in palette],
                       dtype=np.int32)

    # The alternate surface phase re-shades the same pixels through the same
    # fifteen colours, so both phases cost one palette and half the tiles of a
    # second painted copy.
    darker = np.array([c * 0.84 for c in colours])
    partner = np.argmin(((darker[:, None, :] - colours[None, :, :]) ** 2).sum(axis=2),
                        axis=1)
    report["phase_shift"] = {"gain": 0.84,
                             "index_map": [int(v) for v in partner]}

    total_columns = 0
    for name, strip, geometry in strips:
        quantised = quantise_to(hard_alpha(strip), palette)
        index = np.argmin(((neo_quantise(quantised[..., :3])[..., None, :]
                            - colours[None, None, :, :]) ** 2).sum(axis=3), axis=2)
        phases = geometry["phases"]
        width = geometry["width"]
        sheet = np.zeros((geometry["height"], width * phases, 4), dtype=np.int32)
        sheet[:, :width] = quantised
        if phases > 1:
            shifted = quantised.copy()
            shifted[..., :3] = colours[partner[index]]
            sheet[:, width:width * 2] = shifted
        sprites.append(emit(name, sheet, (width, geometry["height"]),
                            (width // 2, 0), 3, palette,
                            {"band": geometry["band"], "phases": phases,
                             "columns_on_screen": geometry["columns_on_screen"]}))
        total_columns += geometry["columns_on_screen"]
    report["on_screen_columns"] = total_columns
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


# ---------------------------------------------------------------- backdrop --

def build_backdrop(sprites: list[dict]) -> dict:
    """One panorama carrying sky, coast and the off-road plane.

    Sky and ground were separate layers with independent parallax until the
    68000 made the cost plain: a full-width layer is twenty hardware sprite
    columns whatever its height, and forty columns of backdrop is a luxury this
    machine cannot pay for alongside the road.  They pan together now.

    The panorama is mirrored about its centre so it wraps without a seam, and
    the ground half is built only from the plate's verge so no second road is
    ever painted behind the real bands.
    """
    note_source(road_strips.PLATE)
    plate = load_rgba(road_strips.PLATE)
    # The plate is composed for a 4:3 game screen: scaling its full height to
    # 224 puts its road vanishing point on our horizon.
    fitted = box_resize(plate, SCREEN_W, SCREEN_H)

    panel = np.zeros((BACKDROP_H, SCREEN_W, 4), dtype=np.float64)
    panel[:HORIZON_Y] = fitted[:HORIZON_Y]

    raw_plate = np.array(Image.open(RAW / road_strips.PLATE).convert("RGB")).astype(np.float64)
    plate_h, plate_w, _ = raw_plate.shape
    squeeze = FUNNEL_K / (road_strips.PLATE_K * road_strips.PLATE_DEPTH_C / 480.0) * 0.5
    for row in range(HORIZON_Y, BACKDROP_H):
        dy = float(row - HORIZON_Y) + 1.0
        plate_dy = float(np.clip(road_strips.PLATE_DEPTH_C * dy / 480.0, 60.0,
                                 plate_h - road_strips.PLATE_VY - 6))
        y = int(round(road_strips.PLATE_VY + plate_dy))
        inner = int(round(road_strips.PLATE_K * plate_dy * 1.34))
        # Only the plate's right-hand verge: the left side runs into the sea,
        # and a strip of ocean tiled across the ground plane reads as a bug.
        verge = raw_plate[y, min(plate_w - 8, int(round(road_strips.PLATE_VX)) + inner):]
        width = max(8, int(round(len(verge) * squeeze)))
        run = np.asarray(Image.fromarray(
            verge[None, :, :].astype(np.uint8)).resize((width, 1), Image.BOX)).astype(np.float64)[0]
        run = np.roll(run, (row * 5) % width, axis=0)
        panel[row, :, :3] = np.resize(run, (SCREEN_W, 3))
        panel[row, :, 3] = 255.0

    panorama = np.concatenate([panel[:, ::-1], panel], axis=1).astype(np.int32)
    palette = pick_palette(panorama)
    sprites.append(emit("backdrop", panorama, (BACKDROP_W, BACKDROP_H), (0, 0), 1, palette))
    return {"height": BACKDROP_H, "panorama_width": BACKDROP_W, "mirrored": True,
            "horizon_row": HORIZON_Y}


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


def build_scenery(sprites: list[dict]) -> dict:
    note_source("ensenada-props.png")
    props = load_rgba("ensenada-props.png")
    out = []
    for index, (name, (col, row), frame, world) in enumerate(SCENERY):
        x0, x1 = PROPS_COLUMNS[col]
        y0, y1 = PROPS_ROWS[row]
        art = fit_sprite(props[y0:y1 + 1, x0:x1 + 1], frame)
        palette = pick_palette(art)
        sprites.append(emit(name, art, frame, (frame[0] // 2, frame[1] - 1),
                            7 + index, palette, {"world_metres": world}))
        out.append({"name": name, "frame": list(frame), "world_metres": world})

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
                        7 + len(SCENERY), dust_palette, {"world_metres": DUST_WORLD_M}))
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


def emit_c_tables(road: dict, scenery: dict, out_dir: Path) -> None:
    """Emit the frame tables the renderer indexes, so art and code cannot drift."""
    bands = road_strips.strip_geometry()
    lines = [
        "/* Generated by tools/build_assets.py.  Do not edit. */",
        '#include "game/bajanew_assets.h"',
        "",
        "const NgSpriteFrame *const bajanew_road_frames"
        "[BAJA_ROAD_BANDS][BAJANEW_ROAD_PHASES] = {",
    ]
    for geometry in bands:
        name = f"road{geometry['band']:02d}"
        second = 1 if geometry["phases"] > 1 else 0
        lines.append(f"    {{&ng_asset_{name}_frames[0], &ng_asset_{name}_frames[{second}]}},")
    lines[-1] = lines[-1].rstrip(",")
    lines += ["};", "",
              "const uint8_t bajanew_road_authored_height[BAJA_ROAD_BANDS] = {"]
    lines.append("    " + ", ".join(str(g["height"]) for g in bands))
    lines += ["};", "", "const BajanewSpriteDef bajanew_scenery[BAJA_SCENERY_KINDS] = {"]
    for name, _cell, _frame, world in SCENERY:
        lines.append(f"    {{&ng_asset_{c_identifier(name)}_frames[0], {int(round(world * 256))}}},")
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


def main() -> None:
    sprites: list[dict] = []
    road = build_road(sprites)
    backdrop = build_backdrop(sprites)
    splash = build_splash(sprites)
    player = build_player(sprites)
    rivals = build_rivals(sprites)
    scenery = build_scenery(sprites)
    fix = build_fix()

    manifest = {
        "format": 2,
        "art_policy": {
            "mode": "bajanew_approved_originals",
            "note": "AGENTS.md forbids reviving Grok; the approved OpenAI raws "
                    "under art/ and the hand-authored FIX glyphs are the only "
                    "shipping sources.",
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
    emit_c_tables(road, scenery, generated)
    write_json(ROOT / "build/assets/CONVERSION.json", {
        "road": road, "backdrop": backdrop, "splash": splash,
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
