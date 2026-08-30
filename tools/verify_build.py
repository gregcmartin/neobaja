#!/usr/bin/env python3
from pathlib import Path
import json
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ROM = ROOT / "build/rom"
expected = {
    "bajanew-p1.p1": 1_048_576,
    "bajanew-c1.c1": 2_097_152,
    "bajanew-c2.c2": 2_097_152,
    "bajanew-s1.s1": 131_072,
    "bajanew-m1.m1": 131_072,
    "bajanew-v1.v1": 524_288,
}
for name, size in expected.items():
    actual = (ROM / name).stat().st_size
    if actual != size:
        raise SystemExit(f"{name}: expected {size}, got {actual}")
with zipfile.ZipFile(ROM / "bajanew.zip") as archive:
    members = {Path(info.filename).name: info.file_size for info in archive.infolist()}
for name, size in expected.items():
    if members.get(name) != size:
        raise SystemExit(f"cartridge member mismatch: {name}")
manifest = json.loads((ROOT / "build/assets/manifest.json").read_text())
if manifest["tile_end"] > 32768:
    raise SystemExit("sprite tile budget exceeded")
print(f"BAJANEW ROM verification: PASS ({manifest['tile_end']} sprite tiles used)")
