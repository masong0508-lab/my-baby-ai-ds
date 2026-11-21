#!/usr/bin/env python3
"""Make the released DS cartridge header identify MY BABY A.I.

The devkitARM ds_rules builder supplies the banner but doesn't expose the
short 12-byte cartridge title/gamecode in this repository. Change those
header bytes and CRC ONLY, after linking. ARM9/ARM7 executable bytes, FAT
resources and legacy compatibility state are not rewritten.
"""
from __future__ import annotations

import argparse
from pathlib import Path

from nds_legacy_header import crc16, u32

TITLE = b"MY BABY A.I."  # Exactly 12 bytes, as required by the DS header.
CODE = b"MBAI"           # Unique homebrew identifier for this project.


def patch_rom(original: bytes) -> bytes:
    if len(original) < 0x4000:
        raise ValueError("Not a complete Nintendo DS ROM")
    if crc16(original[:0x15e]) != int.from_bytes(
        original[0x15e:0x160], "little"
    ):
        raise ValueError("Input Nintendo DS header checksum mismatch")
    regions = [(u32(original, 0x20), u32(original, 0x2c)),
               (u32(original, 0x30), u32(original, 0x3c))]
    for offset, size in regions:
        if offset < 0x200 or size <= 0 or offset + size > len(original):
            raise ValueError("ARM executable lies outside ROM")
    result = bytearray(original)
    result[:12] = TITLE
    result[0x0c:0x10] = CODE
    result[0x15e:0x160] = crc16(result[:0x15e]).to_bytes(2, "little")
    assert all(result[o:o+n] == original[o:o+n] for o, n in regions)
    assert result[0x160:] == original[0x160:]
    return bytes(result)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    args = parser.parse_args()
    original = args.rom.read_bytes()
    changed = patch_rom(original)
    if changed != original:
        args.rom.write_bytes(changed)
    print(f"Cartridge header: {TITLE.decode()} [{CODE.decode()}], "
          f"{len(changed):,} bytes, program preserved")


if __name__ == "__main__":
    main()
