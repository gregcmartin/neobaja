#!/usr/bin/env python3
"""Static release checks for the BAJANEW cartridge set.

BAJANEW authors no static Forge68 scenes, so the SDK's scene budget gate does
not apply and is deliberately not reported here.  The sprite budget is measured
instead from a real run of the game's own renderer.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import zipfile
from pathlib import Path

FAILURES: list[str] = []


def require(condition: bool, message: str) -> None:
    if not condition:
        FAILURES.append(message)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--asset-report", type=Path, required=True)
    parser.add_argument("--audio-report", type=Path, required=True)
    parser.add_argument("--render-report", type=Path, required=True)
    parser.add_argument("--size-tool", default="m68k-elf-size")
    args = parser.parse_args()

    rom = args.rom_dir / "puzzledp.zip"
    require(rom.is_file(), "cartridge zip is missing")
    if rom.is_file():
        with zipfile.ZipFile(rom) as archive:
            names = sorted(archive.namelist())
        for suffix in ("p1", "s1", "m1", "v1", "c1", "c2"):
            require(any(n.endswith(suffix) for n in names),
                    f"cartridge is missing its {suffix.upper()} region")

    assets = json.loads(args.asset_report.read_text(encoding="utf-8"))
    require(assets["c_bytes_used_per_chip"] <= assets["c_bytes_capacity_per_chip"],
            "sprite data exceeds the C-ROM budget")
    palettes: dict[int, str] = {}
    for asset in assets["assets"]:
        require(asset["soft_alpha_pixels"] == 0,
                f"{asset['name']} ships soft alpha")
        require(asset["opaque_colors"] <= 15,
                f"{asset['name']} exceeds fifteen opaque colours")
        palettes.setdefault(asset["palette"], asset["name"])
    require(len(palettes) >= 10, "the scene collapsed onto too few palettes")
    require(any(a["name"] == "player" and a["frames"] >= 5 for a in assets["assets"]),
            "the player is missing its steering and airborne poses")
    road_bands = sum(1 for a in assets["assets"] if a["name"].startswith("road"))
    require(road_bands >= 6, "the road is missing bands")
    require(any(a["name"].startswith("road") and a["frames"] > 1
                for a in assets["assets"]),
            "no road band carries a second surface phase")

    audio = json.loads(args.audio_report.read_text(encoding="utf-8"))
    require(audio["passed"], "audio content report failed")

    render = json.loads(args.render_report.read_text(encoding="utf-8"))
    require(render["dropped_columns"] == 0, "the renderer dropped sprite columns")
    require(render["dropped_commands"] == 0, "the renderer dropped draw commands")
    require(render["overloaded_scanlines"] == 0, "a scanline exceeded its column budget")
    require(render["active_columns"] <= render["column_capacity"],
            "the sprite table overflowed")
    require(render["peak_scanline_columns"] <= render["scanline_capacity"],
            "peak scanline pressure exceeded the hardware contract")
    require(render["moving_frames"] > 400, "the run never moved")
    require(render["offroad_frames"] > 0, "the run never left the road")
    require(render["far_road_x"][1] - render["far_road_x"][0] >= 40,
            "the distant road never swung with a bend")
    require(render["far_road_y"][1] - render["far_road_y"][0] >= 8,
            "the distant road never rose or fell with the terrain")
    require(render["phases_seen"] & 0x1f == 0x1f,
            "the run did not pass through splash, title, select, countdown and race")

    if args.elf.is_file():
        try:
            size = subprocess.run([args.size_tool, str(args.elf)],
                                  capture_output=True, text=True, check=True)
            print(size.stdout.strip())
        except (OSError, subprocess.CalledProcessError) as error:
            FAILURES.append(f"could not size the ELF: {error}")
    else:
        FAILURES.append("linked ELF is missing")

    if FAILURES:
        for failure in FAILURES:
            print(f"FAIL: {failure}", file=sys.stderr)
        raise SystemExit(1)
    print(f"PASS: cartridge set verified "
          f"({assets['unique_tiles']} tiles, "
          f"{render['active_columns']}/{render['column_capacity']} columns, "
          f"peak {render['peak_scanline_columns']}/{render['scanline_capacity']} "
          f"on line {render['peak_scanline']})")


if __name__ == "__main__":
    main()
