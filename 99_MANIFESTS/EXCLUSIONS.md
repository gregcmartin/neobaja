# Extraction exclusions

BAJANEW was intentionally created as a documentation-and-reference packet.
The source project at `/Users/gregmartin/Desktop/BAJA Outrun` was left in place
and was not modified by this extraction.

## Not copied

- Implementation source under `engine/`, `game/`, `include/`, `target/`, and
  `host/`.
- Tests, tools, Makefile, linker/startup implementation, and build config.
- ROMs, BIOS files, object files, archives, logs, NVRAM, emulator save/config
  state, and other executable output.
- Generated art raws under `art/raw/`.
- Converted/shipping art under `assets/source/`.
- Build previews and evidence, except the selected screenshots quarantined in
  `03_REJECTED_IMPLEMENTATION/screenshots/`.
- External SDK, sibling project, OutRun checkout, or other repository code.
- Rollback archives from the old project.

## Documentation archive scope

`90_SOURCE_DOCUMENTATION_ARCHIVE/` contains every Markdown, text, and JSON file
found outside `build/` and `nvram/`, plus the old LICENSE, with relative paths
preserved. Some JSON files describe rejected implementation assets and scenes;
their presence is archival and does not authorize reuse.

## Images copied

- Four primary user mockups, unchanged.
- Eight secondary BAJA: Edge of Control HD reference screenshots, unchanged.
- One user-provided developer splash, unchanged.
- Nine selected MAME screenshots from the rejected implementation, isolated as
  negative evidence.

