# Post-release enhancements

The current cart is playable and passes the software Gauntlet. These are
optional expansion items, not known blockers for the MAME release candidate:

- Verify a prolonged soak on physical AES/MVS hardware and record board/BIOS
  details, controller behavior, audio, video timing, and any sprite-limit faults.
- Add YM2610 engine, impact, skid, crowd, and radio audio; the current V ROM is
  structurally valid but the audio presentation is intentionally minimal.
- Add checkpoints, damage, championship persistence, and a two-player mode.
- Expand Max/Cruz celebration and in-cabin animation, plus additional
  RZR/Maverick contact and airborne poses.
- Add more course-specific dynamic events after re-running the 96-column
  scanline budget and the full frame-synchronous MAME play gate.

Any new bitmap generation must retain the untouched raw, exact prompt, actual
provider provenance, and deterministic conversion. Do not relabel new art as
historical Grok output.
