#!/usr/bin/env python3
"""Run the BAJANEW A/B/steering gate through real MAME input fields."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--rom-dir", type=Path, required=True)
    parser.add_argument("--bios-dir", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--screenshot", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    args.screenshot.parent.mkdir(parents=True, exist_ok=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="bajanew-mame-controls-") as temp:
        temp_path = Path(temp)
        for name in ("cfg", "nvram", "snap"):
            (temp_path / name).mkdir()
        environment = os.environ.copy()
        environment["BAJANEW_SCREENSHOT"] = str(args.screenshot.resolve())
        command = [
            args.mame,
            "puzzledp",
            "-rompath",
            f"{args.rom_dir.resolve()};{args.bios_dir.resolve()}",
            "-bios",
            "euro",
            "-skip_gameinfo",
            "-video",
            "none",
            "-sound",
            "none",
            "-autoboot_delay",
            "0",
            "-autoboot_script",
            str(args.script.resolve()),
            "-cfg_directory",
            str((temp_path / "cfg").resolve()),
            "-nvram_directory",
            str((temp_path / "nvram").resolve()),
            "-snapshot_directory",
            str((temp_path / "snap").resolve()),
            "-seconds_to_run",
            "100",
        ]
        completed = subprocess.run(
            command,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=environment,
            timeout=120,
            check=False,
        )

    log_path = args.output.with_suffix(".log")
    log_path.write_text(completed.stdout, encoding="utf-8")
    failures = re.findall(r"BAJANEW_CONTROL_FAIL ([^\r\n]+)", completed.stdout)
    matches = re.findall(r"BAJANEW_CONTROL_PASS (\{[^\r\n]+\})", completed.stdout)
    if failures:
        raise SystemExit(f"MAME CONTROL FAIL: {failures[-1]} (log: {log_path})")
    if completed.returncode != 0 or not matches:
        raise SystemExit(
            f"MAME CONTROL FAIL: no passing result, exit={completed.returncode} (log: {log_path})"
        )
    result = json.loads(matches[-1])
    if not args.screenshot.is_file():
        raise SystemExit(f"MAME CONTROL FAIL: screenshot missing (log: {log_path})")
    with Image.open(args.screenshot) as image:
        colors = len(set(image.convert("RGB").getdata()))
        if image.size != (320, 224) or colors < 8:
            raise SystemExit(
                f"MAME CONTROL FAIL: screenshot {image.size}, {colors} colors (log: {log_path})"
            )
    evidence = {
        "format": 1,
        "passed": True,
        "input_path": "MAME input-field override to native Neo Geo pad registers",
        "controls": {
            "MAME_P1_Button_1": "throttle",
            "MAME_P1_Button_2": "brake",
            "note": "Literal host-key bindings are verified separately by bajanew-keyboard.json",
        },
        "observations": result,
        "screenshot": str(args.screenshot),
        "screenshot_sha256": sha256(args.screenshot),
        "rom_sha256": sha256(args.rom_dir / "puzzledp.zip"),
        "width": 320,
        "height": 224,
        "colors": colors,
        "log": str(log_path),
    }
    args.output.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(
        "MAME CONTROL PASS: "
        f"A speed={result['throttle_speed']} B drop={result['brake_drop']} "
        f"coast drop={result['coast_drop']} left={result['left_end']} "
        f"right={result['right_end']} screenshot={args.screenshot}"
    )
    print(f"MAME CONTROL EVIDENCE: {args.output}")


if __name__ == "__main__":
    main()
