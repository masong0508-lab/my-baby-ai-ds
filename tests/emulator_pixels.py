"""Read the game's visible bitmap text from a native DS screenshot.

The font and palette come from source/ui.c. This is deliberately a reader
of rendered pixels, not emulator RAM or a duplicate game-state model.
Coordinates refer to the full 256x384 image: add 192 for the bottom screen.
"""
from __future__ import annotations

import ast
from functools import lru_cache
from pathlib import Path
import re


@lru_cache(maxsize=1)
def _font_and_palette():
    source = (Path(__file__).resolve().parents[1] / "source" / "ui.c").read_text()
    table = source.split("static const uint8_t glyph[96][7]={", 1)[1].split("};", 1)[0]
    font = {(0,) * 7: " "}
    for literal, values in re.findall(
        r"\[\s*('(?:\\.|[^'\\])')\s*-\s*32\s*\]\s*=\s*\{([^}]+)\}", table
    ):
        char = ast.literal_eval(literal)
        rows = tuple(int(value.strip()) for value in values.split(","))
        if len(rows) != 7 or any(value < 0 or value > 31 for value in rows):
            raise ValueError(f"Invalid UI glyph: {literal}")
        if rows in font:
            raise ValueError(f"Ambiguous UI glyphs: {char!r} and {font[rows]!r}")
        font[rows] = char
    if len(font) < 50:
        raise ValueError("Could not read the production UI font")
    palette = {
        name.lower(): tuple(int(value) for value in values)
        for name, *values in re.findall(
            r"#define\s+(\w+)\s+RGB\((\d+),(\d+),(\d+)\)", source
        )
    }
    return font, palette


def read_text(image, x: int, y: int, scale: int = 1,
              max_chars: int = 36, ink="dark") -> str:
    """Decode a known text row, leaving unknown glyphs as U+FFFD.

    ``ink`` may be ``"dark"`` for dark text on light cards, a palette name
    such as ``"purple"`` or ``"teal"``, or an RGB tuple. Palette matching
    uses five-bit channels to tolerate either DS-to-RGB expansion scheme.
    """
    if scale < 1 or max_chars < 0 or x < 0 or y < 0:
        raise ValueError("Invalid text coordinates, scale or character count")
    if y + 7 * scale > image.height or x > image.width:
        raise ValueError("Text row falls outside the screenshot")
    font, palette = _font_and_palette()
    if ink == "dark":
        def is_ink(pixel):
            r, g, b = pixel[:3]
            return r < 112 and g < 112 and b < 144
    else:
        target = palette[ink.lower()] if isinstance(ink, str) else tuple(v >> 3 for v in ink)

        def is_ink(pixel):
            return tuple(v >> 3 for v in pixel[:3]) == target

    count = min(max_chars, max(0, (image.width - x + scale) // (6 * scale)))
    pixels = image.load()
    result = []
    for character in range(count):
        left = x + character * 6 * scale
        rows = []
        for row in range(7):
            bits = 0
            for column in range(5):
                pixel = pixels[left + column * scale + scale // 2,
                               y + row * scale + scale // 2]
                if is_ink(pixel):
                    bits |= 1 << (4 - column)
            rows.append(bits)
        result.append(font.get(tuple(rows), "\ufffd"))
    return "".join(result).rstrip()
