#!/usr/bin/env python3
from pathlib import Path
import sys

output = Path(sys.argv[1])
limit = int(sys.argv[2], 0)
payload = bytearray(256 * 64)
for name in sys.argv[3:]:
    payload.extend(Path(name).read_bytes())
if len(payload) > limit:
    raise SystemExit(f"C-ROM payload {len(payload)} exceeds {limit}")
payload.extend(bytes(limit - len(payload)))
output.write_bytes(payload)
