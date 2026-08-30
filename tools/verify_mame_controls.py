#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "P1 A": "KEYCODE_A",
    "P1 B": "KEYCODE_B",
    "P1 Left": "KEYCODE_LEFT",
    "P1 Right": "KEYCODE_RIGHT",
    "P1 Start": "KEYCODE_ENTER",
}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--bios-dir", required=True)
    args = parser.parse_args()

    command = [
        args.mame,
        "-video", "none",
        "-sound", "none",
        "-nothrottle",
        "-skip_gameinfo",
        "-noautosave",
        "-ctrlrpath", str(ROOT / "mame/ctrlr"),
        "-ctrlr", "bajanew_keyboard",
        "-hash", str(ROOT / "build/rom"),
        "-rp", f"{ROOT / 'build/rom'};/opt/homebrew/share/ngdevkit;{Path(args.bios_dir).resolve()}",
        "-autoboot_script", str(ROOT / "tools/inspect_mame_controls.lua"),
        "aes",
        "-cart", "bajanew",
    ]
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    output = result.stdout + result.stderr
    if result.returncode:
        raise SystemExit(output)

    found = {}
    for line in output.splitlines():
        if not line.startswith("BAJANEW CONTROL "):
            continue
        for label in EXPECTED:
            marker = f" {label} type="
            if marker in line:
                found[label] = line.rsplit(" seq=", 1)[-1]
    for label, expected in EXPECTED.items():
        actual = found.get(label)
        if actual != expected:
            raise SystemExit(f"{label}: expected {expected}, got {actual!r}")
    print("BAJANEW keyboard controls: PASS (Enter, arrows, A throttle, B brake)")


if __name__ == "__main__":
    main()
