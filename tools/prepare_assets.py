#!/usr/bin/env python3
"""Deterministically convert retained raw art into Neo Geo tile inputs."""

from __future__ import annotations

import hashlib
import json
from collections import deque
from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / "art/raw/openai/01a05452-0194-7532-93b8-bca5bd770d7c"
OUT = ROOT / "build/assets"
RESAMPLE = Image.Resampling.NEAREST
NAMES = (
    "splash",
    "horizon",
    "player",
    "rival",
    "portraits",
    "roadtiles",
    "props",
)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def remove_light_background(image: Image.Image) -> Image.Image:
    """Remove only the neutral checkerboard baked into two provider outputs."""
    rgba = image.convert("RGBA")
    pixels = list(rgba.get_flattened_data())
    cleaned = []
    for r, g, b, _ in pixels:
        neutral = max(r, g, b) - min(r, g, b) <= 12
        alpha = 0 if neutral and min(r, g, b) >= 226 else 255
        cleaned.append((r, g, b, alpha))
    rgba.putdata(cleaned)
    return rgba


def trim(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")
    width, height = rgba.size
    source = alpha.load()
    seen = bytearray(width * height)
    components = []
    for y in range(height):
        for x in range(width):
            pos = y * width + x
            if seen[pos] or source[x, y] < 128:
                seen[pos] = 1
                continue
            queue = deque([(x, y)])
            seen[pos] = 1
            component = []
            while queue:
                px, py = queue.popleft()
                component.append((px, py))
                for nx, ny in ((px - 1, py), (px + 1, py), (px, py - 1), (px, py + 1)):
                    if 0 <= nx < width and 0 <= ny < height:
                        npos = ny * width + nx
                        if not seen[npos]:
                            seen[npos] = 1
                            if source[nx, ny] >= 128:
                                queue.append((nx, ny))
            components.append(component)
    if not components:
        return rgba
    largest = max(len(component) for component in components)
    keep = [component for component in components if len(component) >= max(8, largest // 80)]
    mask = Image.new("L", rgba.size, 0)
    mask_pixels = mask.load()
    for component in keep:
        for x, y in component:
            mask_pixels[x, y] = source[x, y]
    rgba.putalpha(mask)
    bbox = mask.getbbox()
    return rgba.crop(bbox) if bbox else rgba


def fit_on_canvas(image: Image.Image, size: tuple[int, int], inset: int = 2) -> Image.Image:
    image = trim(image)
    max_w, max_h = size[0] - inset * 2, size[1] - inset * 2
    scale = min(max_w / image.width, max_h / image.height)
    target = (max(1, int(image.width * scale)), max(1, int(image.height * scale)))
    image = image.resize(target, RESAMPLE)
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.alpha_composite(image, ((size[0] - image.width) // 2, size[1] - image.height - inset))
    return canvas


def quantized_gif(image: Image.Image, path: Path) -> None:
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")
    rgb = Image.new("RGB", rgba.size, (0, 0, 0))
    rgb.paste(rgba.convert("RGB"), mask=alpha)
    quant = rgb.quantize(colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    source_palette = quant.getpalette()[: 15 * 3]
    palette = [0, 0, 0] + source_palette + [0] * (768 - 3 - len(source_palette))
    out = Image.new("P", rgba.size, 0)
    out.putpalette(palette)
    qdata = list(quant.get_flattened_data())
    adata = list(alpha.get_flattened_data())
    out.putdata([value + 1 if a >= 128 else 0 for value, a in zip(qdata, adata)])
    out.info["transparency"] = 0
    out.save(path, transparency=0, optimize=False)


def split_cells(image: Image.Image, columns: int, rows: int) -> list[Image.Image]:
    cells = []
    for row in range(rows):
        top = round(row * image.height / rows)
        bottom = round((row + 1) * image.height / rows)
        for column in range(columns):
            left = round(column * image.width / columns)
            right = round((column + 1) * image.width / columns)
            cells.append(image.crop((left, top, right, bottom)))
    return cells


def make_font() -> None:
    source = Image.open(ROOT / "third_party/unscii/unscii8.png").convert("RGB")
    crop = source.crop((2, 1, 258, 25))
    atlas = Image.new("P", (2048, 16), 0)
    atlas.putpalette([0, 0, 0, 255, 255, 255] + [0] * 762)
    for row in range(3):
        for column in range(32):
            char = 32 + row * 32 + column
            glyph = crop.crop((column * 8, row * 8, column * 8 + 8, row * 8 + 8)).convert("L")
            tile = Image.new("P", (8, 8), 0)
            tile.putpalette(atlas.getpalette())
            tile.putdata([1 if value > 64 else 0 for value in glyph.get_flattened_data()])
            atlas.paste(tile, (char * 8, 0))
    bios_messages = (
        ("16-BIT POWERED ",
         ((0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 0x5E, 0x60, 0x7D),
          (0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x40, 0x5F, 0x7C, 0x7E))),
        ("GAME DEVELOPMENT",
         ((0x7F, 0x9A, 0x9C, 0x9E, 0xFF, 0xBB, 0xBD, 0xBF, 0xDA, 0xDC, 0xDE, 0xFA, 0xFC, 0x100, 0x102, 0x104, 0x106),
          (0x99, 0x9B, 0x9D, 0x9F, 0xBA, 0xBC, 0xBE, 0xD9, 0xDB, 0xDD, 0xDF, 0xFB, 0xFD, 0x101, 0x103, 0x105, 0x107))),
    )
    for text, rows in bios_messages:
        for char, top_tile, bottom_tile in zip(text, rows[0], rows[1]):
            glyph = atlas.crop((ord(char) * 8, 0, ord(char) * 8 + 8, 8)).resize((8, 16), RESAMPLE)
            for tile_index, yoff in ((top_tile, 0), (bottom_tile, 8)):
                xpos = (tile_index % 256) * 8
                ypos = (tile_index // 256) * 8
                atlas.paste(glyph.crop((0, yoff, 8, yoff + 8)), (xpos, ypos))
    for empty_tile in (0x40, 0x7D, 0x7E, 0x7F, 0xFF):
        xpos = (empty_tile % 256) * 8
        ypos = (empty_tile // 256) * 8
        atlas.paste(0, (xpos, ypos, xpos + 8, ypos + 8))
    atlas.save(OUT / "font.gif", optimize=False)


def make_splash() -> Image.Image:
    source = Image.open(ROOT / "02_REFERENCE_LIBRARY/developer-splash/devsplashlogo.jpg").convert("RGBA")
    logo = source.resize((216, 216), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (320, 224), (3, 9, 18, 255))
    canvas.alpha_composite(logo, ((320 - 216) // 2, 4))
    return canvas


def make_horizon() -> Image.Image:
    source = Image.open(RAW / "ensenada-full-environment.png").convert("RGBA")
    frame = ImageOps.fit(source, (320, 224), method=RESAMPLE)
    return frame.crop((0, 0, 320, 80))


def make_player() -> Image.Image:
    source = Image.open(RAW / "player-xr22-sheet.png").convert("RGBA")
    cells = split_cells(source, 5, 1)
    canvas = Image.new("RGBA", (128, 96 * 5), (0, 0, 0, 0))
    for index, cell in enumerate(cells):
        canvas.alpha_composite(fit_on_canvas(cell, (128, 96), 1), (0, index * 96))
    return canvas


def make_rival() -> Image.Image:
    source = remove_light_background(Image.open(RAW / "rival-orange-sheet.png"))
    cells = split_cells(source, 4, 1)
    widths = (28, 46, 64, 74)
    canvas = Image.new("RGBA", (80, 64 * 4), (0, 0, 0, 0))
    for index, (cell, width) in enumerate(zip(cells, widths)):
        cell = trim(cell)
        height = min(60, max(1, int(cell.height * width / cell.width)))
        cell = cell.resize((width, height), RESAMPLE)
        frame = Image.new("RGBA", (80, 64), (0, 0, 0, 0))
        frame.alpha_composite(cell, ((80 - width) // 2, 62 - height))
        canvas.alpha_composite(frame, (0, index * 64))
    return canvas


def make_portraits() -> Image.Image:
    source = remove_light_background(Image.open(RAW / "max-cruz-select-v2.png"))
    cells = split_cells(source, 2, 1)
    canvas = Image.new("RGBA", (128, 256), (0, 0, 0, 0))
    for index, cell in enumerate(cells):
        canvas.alpha_composite(fit_on_canvas(cell, (128, 128), 1), (0, index * 128))
    return canvas


def make_road() -> Image.Image:
    """Build four subtle motion phases from the generated full environment."""
    width, frame_height, frames = 576, 144, 4
    source = Image.open(RAW / "ensenada-full-environment.png").convert("RGBA")
    base = ImageOps.fit(source, (320, 224), method=RESAMPLE).crop((0, 80, 320, 224))
    wide = Image.new("RGBA", (width, frame_height), (0, 0, 0, 255))
    left = base.crop((0, 0, 96, frame_height)).transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    right = base.crop((224, 0, 320, frame_height)).transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    wide.alpha_composite(left.resize((128, frame_height), RESAMPLE), (0, 0))
    wide.alpha_composite(base, (128, 0))
    wide.alpha_composite(right.resize((128, frame_height), RESAMPLE), (448, 0))
    image = Image.new("RGBA", (width, frame_height * frames), (0, 0, 0, 255))
    for frame in range(frames):
        phase = wide.copy()
        pixels = phase.load()
        for y in range(frame_height):
            depth = y / (frame_height - 1)
            half = int(10 + 154 * (depth ** 1.32))
            band = max(4, 14 - int(depth * 9))
            for x in range(max(0, 288 - half), min(width, 289 + half)):
                hashed = (x * 2246822519 + y * 3266489917 + frame * 668265263) & 0xFFFFFFFF
                if (y + frame * 3) % band == 0 and (hashed >> 28) == 0:
                    r, g, b, _ = pixels[x, y]
                    pixels[x, y] = (min(255, r + 24), min(255, g + 17), min(255, b + 8), 255)
        image.alpha_composite(phase, (0, frame * frame_height))
    return image


def make_props() -> Image.Image:
    source = Image.open(RAW / "ensenada-props.png").convert("RGBA")
    cells = split_cells(source, 4, 4)
    props = Image.new("RGBA", (32, 32 * 12), (0, 0, 0, 0))
    for index, cell in enumerate(cells[4:]):
        props.alpha_composite(fit_on_canvas(cell, (32, 32), 0), (0, index * 32))
    return props


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    road = make_road()
    props = make_props()
    images = {
        "splash": make_splash(),
        "horizon": make_horizon(),
        "player": make_player(),
        "rival": make_rival(),
        "portraits": make_portraits(),
        "roadtiles": road,
        "props": props,
    }
    for name in NAMES:
        image = images[name]
        assert image.width % 16 == 0 and image.height % 16 == 0
        quantized_gif(image, OUT / f"{name}.gif")
    make_font()

    tile = 256
    offsets = {}
    for name in NAMES:
        image = images[name]
        offsets[name] = tile
        tile += (image.width // 16) * (image.height // 16)

    header = ["#ifndef BAJANEW_ASSETS_GENERATED_H", "#define BAJANEW_ASSETS_GENERATED_H", ""]
    for name, value in offsets.items():
        header.append(f"#define TILE_{name.upper()} {value}u")
    header += [f"#define TILE_END {tile}u", "", "#endif", ""]
    (OUT / "assets.generated.h").write_text("\n".join(header), encoding="utf-8")

    manifest = {
        "provider": "OpenAI built-in image_gen",
        "generation_id": "01a05452-0194-7532-93b8-bca5bd770d7c",
        "conversion": "Pillow nearest-neighbor layout, 15 opaque colors plus transparency, no dithering",
        "raw": {path.name: sha256(path) for path in sorted(RAW.glob("*.png"))},
        "generated": {path.name: sha256(path) for path in sorted(OUT.glob("*.gif"))},
        "tile_offsets": offsets,
        "tile_end": tile,
    }
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
