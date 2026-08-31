#!/usr/bin/env python3
"""Load MAME's tracked BAJANEW profile and verify its resolved key sequences."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--rom-dir", type=Path, required=True)
    parser.add_argument("--bios-dir", type=Path, required=True)
    parser.add_argument("--ctrlr-dir", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="bajanew-keyboard-") as temp:
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
            "-ctrlrpath",
            str(args.ctrlr_dir.resolve()),
            "-ctrlr",
            "bajanew",
            "-cfg_directory",
            str((Path(temp) / "cfg").resolve()),
            "-nvram_directory",
            str((Path(temp) / "nvram").resolve()),
            "-autoboot_delay",
            "0",
            "-autoboot_script",
            str(args.script.resolve()),
            "-seconds_to_run",
            "10",
        ]
        completed = subprocess.run(
            command,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=20,
            check=False,
        )

    log_path = args.output.with_suffix(".log")
    log_path.write_text(completed.stdout, encoding="utf-8")
    failures = re.findall(r"BAJANEW_KEYBOARD_FAIL ([^\r\n]+)", completed.stdout)
    passes = re.findall(r"BAJANEW_KEYBOARD_PASS (\{[^\r\n]+\})", completed.stdout)
    if failures:
        raise SystemExit(f"MAME KEYBOARD FAIL: {failures[-1]} (log: {log_path})")
    if completed.returncode != 0 or not passes:
        raise SystemExit(
            f"MAME KEYBOARD FAIL: no passing result, exit={completed.returncode} (log: {log_path})"
        )
    bindings = json.loads(passes[-1])
    evidence = {
        "format": 1,
        "passed": True,
        "profile": str(args.ctrlr_dir / "bajanew.cfg"),
        "bindings": bindings,
        "log": str(log_path),
    }
    args.output.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(
        "MAME KEYBOARD PASS: "
        f"A={bindings['a']} B={bindings['b']} Start={bindings['start']} "
        f"Left={bindings['left']} Right={bindings['right']}"
    )


if __name__ == "__main__":
    main()
