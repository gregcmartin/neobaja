#!/usr/bin/env python3
from pathlib import Path
import sys

path = Path(sys.argv[1])
size = int(sys.argv[2], 0)
data = path.read_bytes() if path.exists() else b""
if len(data) > size:
    raise SystemExit(f"{path} is {len(data)} bytes, larger than {size}")
path.write_bytes(data + bytes([0xFF]) * (size - len(data)))
