#!/usr/bin/env python3
"""Check that a full-screen asset lands on the screen pixel for pixel.

The developer splash is authored at exactly 320x224, so its capture is a direct
test of the sprite-to-scanline mapping.  A renderer that biases sprite Y drops a
band of hardware backdrop across the top of every frame and clips the same
amount off the bottom, which is easy to miss on art that has margins.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--asset", type=Path, required=True)
    parser.add_argument("--max-error", type=float, default=6.0)
    args = parser.parse_args()

    capture = np.array(Image.open(args.capture).convert("RGB")).astype(np.int32)
    asset = np.array(Image.open(args.asset).convert("RGB")).astype(np.int32)
    if capture.shape != asset.shape:
        raise SystemExit(f"FAIL: capture is {capture.shape[1]}x{capture.shape[0]}, "
                         f"asset is {asset.shape[1]}x{asset.shape[0]}")

    aligned = float(np.abs(capture - asset).mean())
    # If some other vertical shift fits better, the mapping is off by that much.
    best_shift, best_error = 0, aligned
    for shift in range(-24, 25):
        if shift == 0:
            continue
        if shift > 0:
            error = float(np.abs(capture[shift:] - asset[:-shift]).mean())
        else:
            error = float(np.abs(capture[:shift] - asset[-shift:]).mean())
        if error < best_error:
            best_shift, best_error = shift, error

    if best_shift != 0:
        raise SystemExit(
            f"FAIL: the frame is offset by {best_shift} scanlines "
            f"(error {aligned:.2f} aligned, {best_error:.2f} at that shift)")
    if aligned > args.max_error:
        raise SystemExit(f"FAIL: mean channel error {aligned:.2f} exceeds "
                         f"{args.max_error}")
    print(f"PASS: full-screen asset lands on scanline 0 "
          f"(mean channel error {aligned:.2f})")


if __name__ == "__main__":
    main()
