# BAJANEW sound

The cartridge carries its own Z80 driver and V-ROM; nothing is borrowed from
the SDK's demo driver, which only ever played a four-note arpeggio.

## Pieces

- `native/audio/driver.s` - the Z80 driver (SDCC assembly).  NMI queues the
  68000's command byte; the main loop drains the queue with interrupts masked
  around YM2610 writes; the YM2610's Timer B interrupt drives a seven channel
  sequencer (FM 1-4, SSG A-C).  BIOS contract kept: `$01` slot switch from
  RAM, `$03` soft reset, replies are command|`$80`, ready is `$A5`.
- `tools/make_audio.py` - synthesises the engine loop and the effect samples
  deterministically and compiles the V-ROM with the SDK's ADPCM encoder.
- `tools/ngsong.py` - instruments, note tables, the effect and engine tables,
  and the songs, emitted as `build/audio/songdata.s` which the driver includes.
  Songs are written as bar strings, one token per eighth or sixteenth.
- `native/game.c` `bajanew_game_audio()` - one command a frame: effects first
  (crash, contact, go, beep, landing, scrub), then music changes, then the
  engine pitch step when it changed.

## Command map

| Byte | Meaning |
| --- | --- |
| `$02` | stop everything |
| `$10`-`$15` | contact, crash, skid, land, beep, go (ADPCM-A, own channel each) |
| `$20` | music stop |
| `$21` | play "Pacific Run" (Ensenada) |
| `$30` | engine off |
| `$31`-`$6F` | engine pitch step 0..62: ADPCM-B loop delta-N from 0.7x to 2.4x |

The engine step comes from the simulation: revs climb through each of the
five gears and drop at the shift, flare in the air, and wobble off the road.

## Things learned the hard way

- The Z80 boots into the BIOS's SM1 program; the cartridge driver only runs
  after the slot handshake.  SM1 leaves a YM timer flag set, and the interrupt
  line stays low until *both* flags are reset (`$27 = $30`), so a driver that
  only resets Timer B's flag lives in an interrupt storm.  Every tick resets
  both (`$27 = $3A`).
- Timer B's period in MAME is `(256 - TB) * 2304 / 8 MHz`, not the 6144 the
  datasheet arithmetic suggests; `TB = 203` gives the 65.5 Hz tick the songs
  are written for (seven ticks a sixteenth, 140 BPM).
- To watch YM register writes from MAME Lua, tap the Z80 I/O space over
  `0x0000-0xFFFF`: `out (n),a` puts A on the upper address lines, so a tap on
  `0x00-0xFF` sees only writes of zero.
- `make mame-smoke` captures audio with `-sound none -wavwrite`; that works.

## Verifying

`make -C native audio` assembles the driver.  A MAME capture with
`-wavwrite` and a spectrum per second shows the music's notes (A2 bass at
110 Hz, arpeggios at 220-554 Hz), the engine's fundamental sliding with speed
(31 Hz idle to ~140 Hz), and the countdown beeps.  Listening remains Greg's.
