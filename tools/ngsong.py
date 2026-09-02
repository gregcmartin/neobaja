#!/usr/bin/env python3
"""Compile BAJANEW's driver data: note tables, instruments, effects, the
engine pitch table and the songs, as an assembly include for driver.s.

The sequencer runs on the YM2610's Timer B.  Songs are written here as bar
strings, one token per eighth or sixteenth, and compiled to the driver's event
bytes.  Everything is original.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build/audio"

YM_CLOCK = 8_000_000.0
SSG_CLOCK = YM_CLOCK / 4.0
# Timer B period: (256 - TB) * 2304 / 8 MHz seconds, measured in MAME.  203
# gives 65.5 Hz, and seven ticks per sixteenth is 140 BPM.
TICK_PERIOD = 203
TICKS_PER_16TH = 7
ADPCM_B_BASE_DELTA = 0x5555     # 18518 Hz

NOTE_NAMES = {"C": 0, "C#": 1, "DB": 1, "D": 2, "D#": 3, "EB": 3, "E": 4, "F": 5,
              "F#": 6, "GB": 6, "G": 7, "G#": 8, "AB": 8, "A": 9, "A#": 10, "BB": 10, "B": 11}


def note_index(name: str) -> int:
    """Semitones from C0: 'A4' -> 57."""
    letter = name[:-1].upper()
    octave = int(name[-1])
    return octave * 12 + NOTE_NAMES[letter]


def note_hz(index: int) -> float:
    return 440.0 * 2.0 ** ((index - 57) / 12.0)


def fnum_table() -> list[int]:
    """F-numbers for C..B in block 4; block = octave of the note."""
    out = []
    for semitone in range(12):
        f = note_hz(48 + semitone)
        fnum = int(round(f * 144.0 * (1 << 20) / YM_CLOCK / 8.0))
        out.append(fnum)
    return out


def ssg_periods() -> list[int]:
    return [max(1, min(4095, int(round(SSG_CLOCK / (16.0 * note_hz(i)))))) for i in range(96)]


# ------------------------------------------------------------ instruments --
# Operators in musical order op1..op4 as (DT_MUL, TL, KS_AR, AM_DR, SR, SL_RR).
# The YM's register order is op1, op3, op2, op4, handled on emission.
INSTRUMENTS = {
    "bass":  {"alg": 4, "fb": 4, "ops": [(0x01, 0x18, 0x1f, 0x0a, 0x02, 0x37),
                                         (0x01, 0x06, 0x1f, 0x06, 0x03, 0x28),
                                         (0x02, 0x26, 0x1f, 0x0c, 0x02, 0x47),
                                         (0x00, 0x0a, 0x1f, 0x05, 0x02, 0x28)]},
    "lead":  {"alg": 4, "fb": 3, "ops": [(0x02, 0x22, 0x1f, 0x08, 0x02, 0x27),
                                         (0x01, 0x0c, 0x1f, 0x04, 0x01, 0x18),
                                         (0x03, 0x2a, 0x1f, 0x0a, 0x02, 0x37),
                                         (0x01, 0x10, 0x1f, 0x04, 0x01, 0x18)]},
    "organ": {"alg": 7, "fb": 0, "ops": [(0x01, 0x16, 0x14, 0x02, 0x00, 0x08),
                                         (0x02, 0x1c, 0x14, 0x02, 0x00, 0x08),
                                         (0x04, 0x24, 0x14, 0x02, 0x00, 0x08),
                                         (0x00, 0x1a, 0x14, 0x02, 0x00, 0x08)]},
    "kick":  {"alg": 7, "fb": 0, "ops": [(0x00, 0x00, 0x1f, 0x12, 0x10, 0x0f),
                                         (0x01, 0x7f, 0x1f, 0x1f, 0x1f, 0x0f),
                                         (0x01, 0x7f, 0x1f, 0x1f, 0x1f, 0x0f),
                                         (0x01, 0x7f, 0x1f, 0x1f, 0x1f, 0x0f)]},
}
INSTRUMENT_ORDER = ["bass", "lead", "organ", "kick"]


def instrument_bytes(name: str) -> list[int]:
    inst = INSTRUMENTS[name]
    ops = inst["ops"]
    ordered = [ops[0], ops[2], ops[1], ops[3]]
    out = []
    for group in range(7):
        for op in ordered:
            out.append(op[group] if group < 6 else 0x00)
    out.append((inst["fb"] << 3) | inst["alg"])
    out.append(0xc0)
    out += [0, 0]
    assert len(out) == 32
    return out


# ------------------------------------------------------------------ songs --
NOTE_OFF = 0x60
EV_VOLUME = 0x61
EV_INSTRUMENT = 0x62
EV_DRUM = 0x63
EV_END = 0x7f


def wait_bytes(ticks: int) -> list[int]:
    out = []
    while ticks > 0:
        chunk = min(128, ticks)
        out.append(0x7f + chunk)
        ticks -= chunk
    return out


class Track:
    """One channel's event stream, built from tokens on a grid."""

    def __init__(self, steps_per_bar: int) -> None:
        self.data: list[int] = []
        self.step_ticks = TICKS_PER_16TH * 16 // steps_per_bar
        self.pending = 0

    def flush(self) -> None:
        self.data += wait_bytes(self.pending)
        self.pending = 0

    def bars(self, text: str, gate: float = 0.85, transpose: int = 0) -> None:
        """Tokens: note name, '-' hold, '.' rest, or 'x' for a drum (see drums)."""
        tokens = text.split()
        i = 0
        while i < len(tokens):
            token = tokens[i]
            length = 1
            while i + length < len(tokens) and tokens[i + length] == "-":
                length += 1
            ticks = length * self.step_ticks
            if token == ".":
                self.pending += ticks
            else:
                self.flush()
                self.data.append(note_index(token) + transpose)
                on = max(1, int(ticks * gate))
                self.data += wait_bytes(on)
                self.data.append(NOTE_OFF)
                self.pending += ticks - on
            i += length

    def drums(self, text: str, hits: dict[str, tuple[int, int]]) -> None:
        """Tokens map to (noise period, volume); '.' rests."""
        for token in text.split():
            if token == ".":
                self.pending += self.step_ticks
                continue
            period, volume = hits[token]
            self.flush()
            self.data += [EV_VOLUME, volume, EV_DRUM, period]
            self.pending += self.step_ticks

    def instrument(self, index: int) -> None:
        self.flush()
        self.data += [EV_INSTRUMENT, index]

    def volume(self, value: int) -> None:
        self.flush()
        self.data += [EV_VOLUME, value]

    def finish(self) -> list[int]:
        self.flush()
        return self.data + [EV_END]


def ensenada_song() -> list[list[int]]:
    """'Pacific Run': a driving surf-rock loop in A, sixteen bars.

    Chords: A A F#m F#m D D E E, twice, with the lead answering itself the
    second time around.
    """
    bass = Track(8)
    bass.instrument(INSTRUMENT_ORDER.index("bass"))
    bass_line = {
        "A": "A2 A2 A3 A2 A2 A3 A2 E3", "F#m": "F#2 F#2 F#3 F#2 F#2 F#3 F#2 C#3",
        "D": "D3 D3 D4 D3 D3 D4 D3 A3", "E": "E3 E3 E4 E3 E3 E4 E3 B3",
    }
    progression = ["A", "A", "F#m", "F#m", "D", "D", "E", "E"]
    for _ in range(2):
        for chord in progression:
            bass.bars(bass_line[chord], gate=0.7)

    lead = Track(8)
    lead.instrument(INSTRUMENT_ORDER.index("lead"))
    phrase_one = [
        "E5 . C#5 . A4 . B4 .", "C#5 - - - A4 . . .",
        "F#4 . A4 . C#5 . E5 .", "F#5 - - - E5 . . .",
        "D5 . E5 . F#5 . A5 .", "F#5 - - - E5 . D5 .",
        "E5 . D5 . C#5 . B4 .", "E5 - - - - - - -",
    ]
    phrase_two = [
        "A5 . G#5 . E5 . C#5 .", "A5 - - - E5 . C#5 .",
        "F#5 . E5 . C#5 . A4 .", "F#5 - - - - - . .",
        "D5 . F#5 . A5 . B5 .", "A5 - - - F#5 . D5 .",
        "E5 . G#5 . B5 . G#5 .", "E5 - - - - - - -",
    ]
    for bar in phrase_one + phrase_two:
        lead.bars(bar, gate=0.9)

    pad = Track(4)
    pad.instrument(INSTRUMENT_ORDER.index("organ"))
    pad_line = {"A": "C#4 - - -", "F#m": "A3 - - -", "D": "F#3 - - -", "E": "G#3 - - -"}
    for _ in range(2):
        for chord in progression:
            pad.bars(pad_line[chord], gate=0.95)

    kick = Track(4)
    kick.instrument(INSTRUMENT_ORDER.index("kick"))
    for _ in range(16):
        kick.bars("A2 A2 A2 A2", gate=0.5)

    arp_a = Track(16)
    arp_a.volume(8)
    arps = {"A": "A3 C#4 E4 A4", "F#m": "F#3 A3 C#4 F#4", "D": "D3 F#3 A3 D4", "E": "E3 G#3 B3 E4"}
    for _ in range(2):
        for chord in progression:
            notes = arps[chord].split()
            arp_a.bars(" ".join(notes * 4), gate=0.6)

    arp_b = Track(8)
    arp_b.volume(7)
    stabs = {"A": ". E4 . . . E4 . E4", "F#m": ". C#4 . . . C#4 . C#4",
             "D": ". A3 . . . A3 . A3", "E": ". B3 . . . B3 . B3"}
    for _ in range(2):
        for chord in progression:
            arp_b.bars(stabs[chord], gate=0.4)

    hats = Track(8)
    kit = {"h": (2, 6), "s": (12, 13), "o": (4, 9)}
    for bar in range(16):
        pattern = "h h s h h h s o" if bar % 4 != 3 else "h h s h s h s s"
        hats.drums(pattern, kit)

    return [bass.finish(), lead.finish(), pad.finish(), kick.finish(),
            arp_a.finish(), arp_b.finish(), hats.finish()]


SONGS = [("ensenada", ensenada_song)]


# --------------------------------------------------------------- emission --

def db(values: list[int]) -> str:
    lines = []
    for i in range(0, len(values), 16):
        lines.append("        .db     " + ", ".join(f"0x{v & 0xff:02x}" for v in values[i:i + 16]))
    return "\n".join(lines)


def main() -> None:
    report = json.loads((OUT / "gen/audio_report.json").read_text(encoding="utf-8"))
    samples = {s["name"]: s for s in report["samples"]}
    lines = ["; Generated by tools/ngsong.py.  Do not edit.", "",
             f"tick_period:\n        .db     {TICK_PERIOD}", "",
             "fnum_table:"]
    fn = []
    for value in fnum_table():
        fn += [value & 0xff, (value >> 8) & 0x07]
    lines += [db(fn), "", "ssg_period_table:"]
    sp = []
    for value in ssg_periods():
        sp += [value & 0xff, (value >> 8) & 0x0f]
    lines += [db(sp), "", "effect_table:"]
    effects = [s for s in report["samples"] if s["codec"] == "adpcm_a"]
    table = []
    for index in range(16):
        if index < len(effects):
            s = effects[index]
            channel_bit = 1 << index
            table += [channel_bit, 0xdf, s["start_page"] & 0xff, s["start_page"] >> 8,
                      s["end_page"] & 0xff, s["end_page"] >> 8]
        else:
            table += [0, 0, 0, 0, 0, 0]
    lines += [db(table), ""]
    engine = samples["engine"]
    lines += ["engine_start:", db([engine["start_page"] & 0xff, engine["start_page"] >> 8]),
              "engine_end:", db([engine["end_page"] & 0xff, engine["end_page"] >> 8]),
              "engine_level:", db([0xc0]), "", "engine_pitch_table:"]
    pitch = []
    for step in range(63):
        ratio = 0.7 * (2.4 / 0.7) ** (step / 62.0)
        delta = int(round(ADPCM_B_BASE_DELTA * ratio))
        pitch += [delta & 0xff, delta >> 8]
    lines += [db(pitch), "", "instrument_table:"]
    for name in INSTRUMENT_ORDER:
        lines.append(f"; {name}")
        lines.append(db(instrument_bytes(name)))
    while len(INSTRUMENT_ORDER) < 8:
        INSTRUMENT_ORDER.append("kick")
    lines += ["", "song_table:"]
    for name, _ in SONGS:
        lines.append(f"        .dw     song_{name}")
    for name, build in SONGS:
        channels = build()
        lines += ["", f"song_{name}:"]
        for index in range(7):
            lines.append(f"        .dw     song_{name}_ch{index}, song_{name}_ch{index}_loop")
        for index, data in enumerate(channels):
            # The instrument and volume prefix runs once; the loop restarts
            # after it.
            prefix = 0
            while prefix < len(data) and data[prefix] in (EV_INSTRUMENT, EV_VOLUME):
                prefix += 2
            lines.append(f"song_{name}_ch{index}:")
            if prefix:
                lines.append(db(data[:prefix]))
            lines.append(f"song_{name}_ch{index}_loop:")
            lines.append(db(data[prefix:]))
    total = sum(len(build()[i]) for _, build in SONGS for i in range(7))
    (OUT / "songdata.s").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"SONG PASS: {len(SONGS)} song(s), {total} event bytes")


if __name__ == "__main__":
    main()
