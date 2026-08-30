# BAJA Outrun AI handoff

Snapshot time: **2026-08-30 13:15:46 CDT**

This directory is the durable handoff for continuing BAJA Outrun with Codex,
without restarting Grok or UltraGrok. Start with `RESUME_PROMPT.md`, then read
`CURRENT_STATE.md`, `CODEX_HANDOFF.md`, and `GAMEPLAY_ORACLE.md` before editing.

## Prompt archive

- `USER_REQUESTS.md` — the user's requests in chronological order.
- `GAUNTLET_PROMPT.md` — archived complete UltraGrok `/gauntlet` prompt. Its
  quality bar still applies, but its Grok-runtime instructions do not.
- `CODEX_CONTINUATION_PROMPT.md` — Codex-native version of the task with no
  Grok runtime dependency.
- `ORIGINAL_WORKFLOW_OBJECTIVE.md` — the exact objective frozen into the old
  UltraGrok workflow. It predates the expanded OutRun gate.
- `RESUME_PROMPT.md` — short ready-to-paste Codex continuation prompt.
- `art-prompts/` — copies of every shipping-art prompt under `art/prompts/`.
- `GROK_IMAGE_CALLS.json` — every unique `image_gen` / `image_edit` request
  recovered from the builder and repair histories, including failed retries.

## State and evidence

- `CURRENT_STATE.md` — what is complete, what still fails, known risks, exact
  workflow/session IDs, and the next action.
- `WORKFLOW_SNAPSHOT.json` — machine-readable pre-handoff UltraGrok snapshot.
- `FINAL_GROK_STATE.json` — post-shutdown state (`interrupted`, revision 60).
- `SCORECARD_AT_HANDOFF.md` — exact final persisted critic scorecard before the
  public-path cleanup in `docs/GAUNTLET_SCORECARD.md`.
- `GAMEPLAY_ORACLE.md` — clean-room OutRun behavior contract.
- `REFERENCE_MAP.md` — primary/secondary visual references and pinned hashes.
- `VERIFICATION.md` — commands, latest known results, artifacts, and limits.
- `HANDOFF_SHA256SUMS.txt` — 218-file drift/rollback baseline covering the
  project sources, prompts, reference inputs, tooling, tests, ROM, and evidence.

## Writer state

The original UltraGrok process was intentionally stopped at 13:18 CDT. Its
workflow recorded `interrupted`; two round-3 critics were cancelled and one
finished. No Grok or MAME writer remained after shutdown. Codex should still
perform a quick process check before editing, but it should not resume or
relaunch the old workflow.

The target directory is not a Git repository. Preserve user files and make an
explicit backup/snapshot before broad changes.
