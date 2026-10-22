#!/usr/bin/env python3
"""Every Han character in the Chinese menus must be in the built-in UI font."""
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
charset = set((root / "lib/EpdFont/scripts/cjk-ui-charset.txt").read_text(encoding="utf-8"))
headers = [(root / f"lib/EpdFont/builtinFonts/cjk_ui_{size}.h").read_text(encoding="utf-8") for size in (8, 10, 12)]
han = re.compile(r"[㐀-䶿一-鿿豈-﫿]")
failed = False
for path in sorted((root / "lib/I18n/translations").glob("chinese*.yaml")):
    missing = sorted(set(han.findall(path.read_text(encoding="utf-8"))) - charset)
    if missing:
        failed = True
        print(f"{path.name}: add to cjk-ui-charset.txt and run generate-cjk-ui-fonts.py: {''.join(missing)}")
# The generated headers must match the character list they were built from.
for header in headers:
    if not header.count("generate-cjk-ui-fonts.py"):
        failed = True
        print("cjk_ui headers were not produced by generate-cjk-ui-fonts.py")
intervals = [re.findall(r"\{\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+),", h.split("Intervals[] = {")[1].split("};")[0])
             for h in headers]
for size, ranges in zip((8, 10, 12), intervals):
    covered = set()
    for first, last in ranges:
        covered.update(range(int(first, 16), int(last, 16) + 1))
    stale = sorted(ord(c) for c in charset if not c.isspace() and ord(c) not in covered)
    if stale:
        failed = True
        print(f"cjk_ui_{size}.h is stale: {len(stale)} characters missing; run generate-cjk-ui-fonts.py")
sys.exit(1 if failed else 0)
