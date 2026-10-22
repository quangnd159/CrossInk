#!/usr/bin/env python3
"""Make a UI Font TTF that covers every CJK ideograph, for S3 readers.

The built-in Chinese UI glyphs (generate-cjk-ui-fonts.py) cover the ~3,650 most
common simplified characters. A UI Font on the SD card fills in everything
else: rare and traditional characters in file names and book titles.

Noto Sans CJK ships as a CID-keyed CFF font with 65,535 glyphs, which the
reader's font engine cannot open. This keeps the Latin, punctuation, CJK and
kana blocks, and converts the outlines to TrueType quadratics, giving a ~9 MB
static TTF.

Usage:
    python3 make-cjk-ui-ttf.py [SOURCE.otf] [OUTPUT.ttf]

Copy the output to /.crosspoint/languages/fonts/Noto Sans SC/ on the SD card,
then choose it in Settings > System > Device > UI Font.
"""

import sys
from pathlib import Path

from fontTools import subset
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont, newTable

HERE = Path(__file__).resolve().parent
DEFAULT_SOURCE = HERE.parent / "builtinFonts/source/NotoSansCJKsc/NotoSansCJKsc-Regular.otf"
RANGES = [
    (0x0020, 0x024F),  # Latin
    (0x2000, 0x206F),  # general punctuation
    (0x2E80, 0x2FDF),  # radicals
    (0x3000, 0x30FF),  # CJK punctuation, kana
    (0x3400, 0x4DBF),  # Extension A
    (0x4E00, 0x9FFF),  # Unified Ideographs
    (0xF900, 0xFAFF),  # compatibility ideographs
    (0xFE30, 0xFE4F),  # compatibility forms
    (0xFF00, 0xFFEF),  # full-width forms
]


def to_truetype(font):
    order = font.getGlyphOrder()
    glyphs = font.getGlyphSet()
    outlines = {}
    for name in order:
        pen = TTGlyphPen(glyphs)
        glyphs[name].draw(Cu2QuPen(pen, 1.0, reverse_direction=True))
        outlines[name] = pen.glyph()
    font["loca"] = newTable("loca")
    glyf = font["glyf"] = newTable("glyf")
    glyf.glyphOrder = order
    glyf.glyphs = outlines
    del font["CFF "]
    if "VORG" in font:
        del font["VORG"]
    maxp = font["maxp"] = newTable("maxp")
    maxp.tableVersion = 0x00010000
    maxp.numGlyphs = len(order)
    for field in ("maxPoints", "maxContours", "maxCompositePoints", "maxCompositeContours", "maxStorage",
                  "maxTwilightPoints", "maxFunctionDefs", "maxInstructionDefs", "maxStackElements",
                  "maxSizeOfInstructions", "maxComponentElements", "maxComponentDepth"):
        setattr(maxp, field, 0)
    maxp.maxZones = 1
    font["head"].indexToLocFormat = 1
    font["head"].glyphDataFormat = 0
    font["post"].formatType = 3.0
    font.sfntVersion = "\x00\x01\x00\x00"


def main():
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_SOURCE
    output = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("NotoSansSC-UI.ttf")
    font = TTFont(source)
    options = subset.Options()
    options.layout_features = []
    options.hinting = False
    options.name_IDs = ["*"]
    options.notdef_outline = True
    options.desubroutinize = True
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=[cp for first, last in RANGES for cp in range(first, last + 1)])
    subsetter.subset(font)
    to_truetype(font)
    font.save(output)  # recalculates maxp and bounding boxes from the new outlines
    print(f"Wrote {output} ({len(font.getGlyphOrder())} glyphs, {output.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
