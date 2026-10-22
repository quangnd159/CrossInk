#!/usr/bin/env python3
"""Generate the built-in Chinese UI fallback fonts (cjk_ui_8/10/12).

The Inter UI fonts are Latin-only. These fonts fill in Chinese glyphs for menus,
file names and book titles without any SD-card setup. They are attached to the
UI font families as a per-glyph fallback (see main.cpp), so Latin text keeps
Inter and only the missing characters come from here.

The character set lives in cjk-ui-charset.txt (one or more characters per line,
whitespace ignored). It holds 3,654 simplified Hanzi:

- the 3,500 most frequent characters, ranked by summing jieba dict.txt word
  frequencies per character (99% of running text);
- every simplified character in the HSK 3.0 word list (ivankra/hsk30);
- every character used by lib/I18n/translations/chinese-simplified.yaml.

CJK punctuation and full-width forms are always included. Rarer characters are
drawn from the reader's SD-card font when one is selected (whole-string
fallback in GfxRenderer::resolveTextFontId), or by a UI Font on S3 readers.

Glyphs are 1-bit and uncompressed to match the Inter UI fonts' rendering path;
the three sizes cost about 0.8 MB of flash.

Usage (from this directory):
    python3 generate-cjk-ui-fonts.py
"""

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent / "builtinFonts/source/NotoSansCJKsc/NotoSansCJKsc-Regular.otf"
OUTPUT = HERE.parent / "builtinFonts"
SIZES = (8, 10, 12)
ALWAYS = [
    (0x00B7, 0x00B7),  # middle dot
    (0x2014, 0x2026),  # dashes, curly quotes, ellipsis
    (0x3000, 0x303F),  # CJK symbols and punctuation
    (0xFF01, 0xFF5E),  # full-width forms
]


def intervals():
    text = (HERE / "cjk-ui-charset.txt").read_text(encoding="utf-8")
    codepoints = {ord(c) for c in text if not c.isspace()}
    for first, last in ALWAYS:
        codepoints.update(range(first, last + 1))
    ranges = []
    for cp in sorted(codepoints):
        if ranges and cp == ranges[-1][1] + 1:
            ranges[-1][1] = cp
        else:
            ranges.append([cp, cp])
    return ranges


def main():
    args = []
    for first, last in intervals():
        args += ["--additional-intervals", f"{first:#x},{last:#x}"]
    for size in SIZES:
        name = f"cjk_ui_{size}"
        header = subprocess.run(
            [sys.executable, str(HERE / "fontconvert.py"), name, str(size), str(SOURCE),
             "--no-default-intervals", *args],
            stdout=subprocess.PIPE, check=True, text=True,
        ).stdout
        # The interval list is thousands of arguments; point at this script instead.
        lines = [" * Command used: generate-cjk-ui-fonts.py (characters from cjk-ui-charset.txt)"
                 if line.startswith(" * Command used:") else line for line in header.splitlines()]
        (OUTPUT / f"{name}.h").write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"Generated {OUTPUT / name}.h")


if __name__ == "__main__":
    main()
