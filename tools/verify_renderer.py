#!/usr/bin/env python3
"""Prove the Forge68 flush optimisation changes nothing the hardware sees.

Builds the game's host renderer twice - once with the run reuse fast paths and
once with them compiled out - and compares the VRAM digest after every frame of
an identical run.  Any divergence means the optimised flush skipped a write the
full path would have made.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/renderer-equivalence"
SDK = ROOT / "sdk/forge68"

SOURCES = [
    ROOT / "tools/renderer_equivalence.c",
    ROOT / "src/sim.c",
    ROOT / "native/game.c",
    ROOT / "build/assets/generated/assets.c",
    ROOT / "build/assets/generated/bajanew_assets.c",
] + sorted(SDK.glob("engine/*.c"))

INCLUDES = ["-I" + str(ROOT / "include"), "-I" + str(ROOT / "native/include"),
            "-I" + str(SDK / "include"), "-I" + str(ROOT / "build/assets/generated")]


def build(name: str, extra: list[str]) -> Path:
    BUILD.mkdir(parents=True, exist_ok=True)
    binary = BUILD / name
    command = ["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-DNG_HOST"]
    command += INCLUDES + extra + [str(path) for path in SOURCES] + ["-o", str(binary)]
    subprocess.run(command, check=True)
    return binary


def main() -> None:
    fast = subprocess.run([str(build("fast", []))], capture_output=True, text=True, check=True)
    full = subprocess.run([str(build("full", ["-DNG_RENDERER_NO_RUN_REUSE"]))],
                          capture_output=True, text=True, check=True)
    fast_lines = fast.stdout.splitlines()
    full_lines = full.stdout.splitlines()
    if not fast_lines:
        raise SystemExit("FAIL: the harness produced no frames")
    if len(fast_lines) != len(full_lines):
        raise SystemExit("FAIL: the two builds ran a different number of frames")
    for index, (a, b) in enumerate(zip(fast_lines, full_lines)):
        if a != b:
            print(f"FAIL: VRAM diverged at frame {index}", file=sys.stderr)
            print(f"  reuse: {a}", file=sys.stderr)
            print(f"  full:  {b}", file=sys.stderr)
            raise SystemExit(1)
    print(f"PASS: {len(fast_lines)} frames of identical VRAM with and without "
          f"renderer run reuse")


if __name__ == "__main__":
    main()
