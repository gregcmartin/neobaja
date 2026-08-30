#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
manifest_path = ROOT / "build/assets/manifest.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
raw_dir = ROOT / "art/raw/openai" / manifest["generation_id"]
sum_path = raw_dir / "SHA256SUMS"
pinned = {}
for line in sum_path.read_text(encoding="utf-8").splitlines():
    expected, name = line.split(maxsplit=1)
    pinned[name] = expected

if manifest["provider"] != "OpenAI built-in image_gen":
    raise SystemExit("unexpected art provider")
if manifest["raw"] != pinned:
    raise SystemExit("raw manifest does not match pinned SHA256SUMS")
for name, expected in manifest["raw"].items():
    path = raw_dir / name
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f"raw hash mismatch: {name}")
for name, expected in manifest["generated"].items():
    path = ROOT / "build/assets" / name
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f"converted hash mismatch: {name}")
for name in ("splash", "horizon", "player", "rival", "portraits", "roadtiles", "props"):
    if not (ROOT / "build/assets" / f"{name}.gif").is_file():
        raise SystemExit(f"missing converted asset: {name}")

splash = ROOT / "02_REFERENCE_LIBRARY/developer-splash/devsplashlogo.jpg"
if hashlib.sha256(splash.read_bytes()).hexdigest() != "6e01d4f3fdb9daaa6feb90b52ab0497e3f26b5191c6ac205b0efeed1ed6eeba1":
    raise SystemExit("developer splash source hash mismatch")
font = ROOT / "third_party/unscii/unscii8.png"
if hashlib.sha256(font.read_bytes()).hexdigest() != "83135f71202d1702a897c85dbf4ba0886795ce77629b43b2c15468b23402f8ca":
    raise SystemExit("Unscii font source hash mismatch")

print("BAJANEW production art check: PASS")
