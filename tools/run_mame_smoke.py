#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import subprocess

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--bios-dir", required=True)
    args = parser.parse_args()

    bios_dir = Path(args.bios_dir).resolve()
    if not (bios_dir / "neogeo.zip").is_file():
        raise SystemExit(f"missing external Neo Geo BIOS: {bios_dir / 'neogeo.zip'}")

    evidence = ROOT / "evidence/mame-smoke"
    evidence.mkdir(parents=True, exist_ok=True)
    for old in evidence.glob("*.png"):
        old.unlink()

    romdir = ROOT / "build/rom"
    env = os.environ.copy()
    env["BAJANEW_EVIDENCE"] = str(evidence)
    aes_command = [
        args.mame,
        "-video", "none",
        "-sound", "none",
        "-nothrottle",
        "-skip_gameinfo",
        "-noautosave",
        "-hash", str(romdir),
        "-rp", f"{romdir};/opt/homebrew/share/ngdevkit;{bios_dir}",
        "-autoboot_script", str(ROOT / "tools/mame_smoke.lua"),
        "-seconds_to_run", "85",
        "aes",
        "-cart", "bajanew",
    ]
    result = subprocess.run(aes_command, cwd=ROOT, env=env, text=True, capture_output=True)
    (evidence / "aes.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
        raise SystemExit(result.stdout + result.stderr)

    names = (
        "splash", "title", "select-max", "select-cruz", "countdown", "race-a",
        "race-b", "coast", "rival-approach", "rival-contact", "offroad", "late-race",
    )
    hashes = {}
    for name in names:
        path = evidence / f"{name}.png"
        if not path.is_file():
            raise SystemExit(f"MAME did not capture {path.name}; see {evidence / 'aes.log'}")
        with Image.open(path) as image:
            if image.size != (320, 224):
                raise SystemExit(f"{path.name}: expected 320x224, got {image.size}")
            extrema = image.convert("RGB").getextrema()
            if all(low == high for low, high in extrema):
                raise SystemExit(f"{path.name}: blank or flat capture")
        hashes[name] = digest(path)

    if hashes["race-a"] == hashes["race-b"] or hashes["race-b"] == hashes["coast"]:
        raise SystemExit("race captures did not change across real MAME frames")
    if hashes["splash"] == hashes["title"]:
        raise SystemExit("splash and title captures are identical")

    mvs_command = [
        args.mame, "neogeo", "bajanew",
        "-video", "none",
        "-sound", "none",
        "-nothrottle",
        "-skip_gameinfo",
        "-noautosave",
        "-hash", str(romdir),
        "-rp", f"{romdir};/opt/homebrew/share/ngdevkit;{bios_dir}",
        "-autoboot_script", str(ROOT / "tools/mame_mvs_smoke.lua"),
        "-seconds_to_run", "40",
    ]
    result = subprocess.run(mvs_command, cwd=ROOT, env=env, text=True, capture_output=True)
    (evidence / "mvs.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
        raise SystemExit(result.stdout + result.stderr)

    mvs_names = ("mvs-title-before-coin", "mvs-title-after-coin", "mvs-select", "mvs-race")
    mvs_hashes = {}
    for name in mvs_names:
        path = evidence / f"{name}.png"
        if not path.is_file():
            raise SystemExit(f"MAME did not capture {path.name}; see {evidence / 'mvs.log'}")
        with Image.open(path) as image:
            if image.size != (320, 224):
                raise SystemExit(f"{path.name}: expected 320x224, got {image.size}")
        mvs_hashes[name] = digest(path)
    if mvs_hashes["mvs-title-before-coin"] != mvs_hashes["mvs-title-after-coin"]:
        raise SystemExit("MVS coin insertion unexpectedly changed the waiting title")
    if mvs_hashes["mvs-title-after-coin"] == mvs_hashes["mvs-select"]:
        raise SystemExit("MVS Start did not reach racer selection")
    if mvs_hashes["mvs-select"] == mvs_hashes["mvs-race"]:
        raise SystemExit("MVS selection did not reach the race")

    total = len(names) + len(mvs_names)
    print(f"BAJANEW MAME smoke: PASS ({total} AES/MVS captures in {evidence})")


if __name__ == "__main__":
    main()
