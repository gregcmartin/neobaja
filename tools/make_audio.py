#!/usr/bin/env python3
"""Synthesise BAJANEW's sample content and compile it into the V-ROM.

Every sample is generated deterministically here - the engine loop, contact
and crash noise, the landing thump, the countdown beeps - then encoded by the
SDK's ADPCM compiler.  Nothing is recorded from a third party.
"""
from __future__ import annotations

import json
import math
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "sdk/forge68/tools"))
import ngaudio  # noqa: E402

RATE = 18518
OUT = ROOT / "build/audio"
WAV = OUT / "wav"

# Command numbers shared with the driver and the game.
EFFECT_BASE = 0x10
ENGINE_COMMAND = 0x31


def normalise(pcm: list[float], peak: float) -> list[int]:
    top = max(1e-6, max(abs(v) for v in pcm))
    return [int(round(v / top * peak * 32767)) for v in pcm]


def engine_loop() -> list[int]:
    """Eight periods of a rough four-stroke idle: harmonics that fall off
    slowly, a soft exhaust pop each period, and a little grit."""
    period = 320   # eight periods make exactly five 256-byte pages: a seamless loop
    periods = 8
    rng = random.Random(7)
    out = []
    for i in range(period * periods):
        t = (i % period) / period
        v = 0.0
        for k in range(1, 11):
            v += math.sin(2 * math.pi * k * t + 0.4 * k) / (k ** 0.85)
        pop = math.exp(-t * 18.0) * math.sin(2 * math.pi * 6.0 * t)
        v += 0.9 * pop
        v += 0.12 * (rng.random() * 2 - 1)
        out.append(v)
    return normalise(out, 0.7)


def noise_burst(seconds: float, decay: float, seed: int, lowpass: int = 1,
                thump_hz: float = 0.0, thump_gain: float = 0.0) -> list[int]:
    rng = random.Random(seed)
    count = int(RATE * seconds)
    raw = [rng.random() * 2 - 1 for _ in range(count)]
    if lowpass > 1:
        acc = 0.0
        smooth = []
        for v in raw:
            acc += (v - acc) / lowpass
            smooth.append(acc)
        raw = smooth
    out = []
    for i, v in enumerate(raw):
        t = i / RATE
        env = math.exp(-t * decay)
        sample = v * env
        if thump_gain > 0.0:
            sample += thump_gain * math.exp(-t * decay * 1.5) * math.sin(2 * math.pi * thump_hz * t)
        out.append(sample)
    return normalise(out, 0.9)


def tone(seconds: float, hz: float, attack: float = 0.004, release: float = 0.03) -> list[int]:
    count = int(RATE * seconds)
    out = []
    for i in range(count):
        t = i / RATE
        env = min(1.0, t / attack) * min(1.0, (seconds - t) / release)
        out.append(env * (0.8 * math.sin(2 * math.pi * hz * t) + 0.2 * math.sin(2 * math.pi * hz * 2 * t)))
    return normalise(out, 0.8)


EFFECTS = [
    ("contact", lambda: noise_burst(0.16, 22.0, 11, lowpass=2, thump_hz=140.0, thump_gain=0.6)),
    ("crash", lambda: noise_burst(0.38, 9.0, 13, lowpass=3, thump_hz=60.0, thump_gain=0.9)),
    ("skid", lambda: noise_burst(0.32, 8.0, 17, lowpass=6)),
    ("land", lambda: noise_burst(0.14, 26.0, 19, lowpass=8, thump_hz=70.0, thump_gain=1.0)),
    ("beep", lambda: tone(0.09, 880.0)),
    ("go", lambda: tone(0.28, 1318.5)),
]


def main() -> None:
    WAV.mkdir(parents=True, exist_ok=True)
    samples = []
    for index, (name, make) in enumerate(EFFECTS):
        ngaudio.write_wav(WAV / f"{name}.wav", make(), RATE)
        samples.append({"name": name, "codec": "adpcm_a", "source": f"wav/{name}.wav",
                        "sample_rate_hz": RATE, "command": EFFECT_BASE + index})
    ngaudio.write_wav(WAV / "engine.wav", engine_loop(), RATE)
    samples.append({"name": "engine", "codec": "adpcm_b", "source": "wav/engine.wav",
                    "sample_rate_hz": RATE, "command": ENGINE_COMMAND})
    manifest = {"format_version": 1, "v_rom_bytes": 524288, "samples": samples,
                "synth_commands": {"stop_all": 2, "music_stop": 0x20, "music_ensenada": 0x21,
                                   "engine_off": 0x30}}
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    report = ngaudio.compile_manifest(OUT / "manifest.json", OUT / "gen")
    print(f"AUDIO PASS: {len(report['samples'])} samples, {report['bytes_used']} V-ROM bytes")


if __name__ == "__main__":
    main()
