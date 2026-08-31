# Forge68 testing integration

Greg selected the enhanced Forge68 Neo Geo SDK as BAJANEW's native platform and
AI-assisted verification layer on 2026-08-30.

## Inspected SDK state

- Local checkout: `/Users/gregmartin/Desktop/neogeo`
- Commit: `f52c1a0` (`fix: make release checks portable`)
- Upstream: `https://github.com/gregcmartin/ng_neogeo_SDK`
- License: MIT

The SDK provides the Neo Geo BIOS/runtime layer, sprite renderer, asset and scene
compilers, ROM packing, target telemetry, host build, MAME gates, audio, and the
local `forge68` MCP server. BAJANEW does not reuse Forge68's demo gameplay,
tuning, or scene content. Its fixture sprites are temporary diagnostic
stand-ins in the programming shell only and cannot ship as BAJANEW game art.

## Current integration status

The SDK is pinned as `sdk/forge68` at the commit above. BAJANEW currently uses
its native runtime, renderer, asset compiler, ROM packer/verifier, host platform,
target telemetry, MAME smoke tools, and input-field control technique. The
enhanced MCP server and persistent AI host protocol were inspected and recorded
on 2026-08-30, including deterministic named inputs, structured state, debug
screenshots, persistent MAME control, and bounded evidence bundles.

The existing MCP executable still describes and builds the SDK demo. A
BAJANEW-specific state serializer/adapter is therefore an open implementation
gate. Until that adapter exists and passes against this game, no SDK demo MCP
result may be reported as BAJANEW gameplay evidence.

## Required AI test loop

When the project-scoped Forge68 MCP server is wired into BAJANEW:

1. inspect `sdk_status`;
2. start the deterministic host with autoplay disabled;
3. inject exact named input sequences with `host_step`;
4. inspect `host_state`, the structured debug log, and a debug-overlay host
   screenshot;
5. build the cartridge profile;
6. repeat the same controls in persistent MAME;
7. require native 320x224 screenshots and advancing cartridge telemetry;
8. preserve the final unscripted five-minute human MAME gate.

## BAJANEW overrides

- Normal title, selection, countdown, and gameplay never enable autoplay.
- AI input is a test-only external control path. It is not compiled as idle
  throttle or hidden gameplay input.
- Any attract mode remains disabled until Greg separately approves it.
- The SDK's Grok-only production-art policy is not used for BAJANEW because this
  project's `AGENTS.md` explicitly forbids reviving Grok. The approved original
  OpenAI art and its existing provider/model/hash records remain authoritative.
- Skipping that one incompatible production-art gate must be reported honestly;
  host, sanitizer, ROM, telemetry, sprite-budget, input, and MAME gates remain
  required.
