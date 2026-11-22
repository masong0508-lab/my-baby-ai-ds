#!/usr/bin/env python3
"""Convert this project's DS-only ROM into a plain homebrew-loader layout.

Both supported toolchains currently place ARM9 at ROM 0x4000, with an
unused 0x800-byte secure-area prefix. Some older emulators treat that
retail-like placement as an encrypted cartridge. Rather than invoking
retail-game decryption, copy the EXACT compiled ARM9 bytes to 0x1000
(4 KiB aligned, below the secure-area threshold), and update only
the header's ARM9 source offset, header size, secure CRC and header CRC.

ARM7, FNT, FAT, banner and all file data stay at their existing offsets.
This does not certify DraStic without testing it there. It makes an
additional ROM; the existing native-tested ROM stays unmodified.
"""
from __future__ import annotations

import argparse
from pathlib import Path

ARM9_PLAIN_OFFSET = 0x1000
ARM9_RETAIL_OFFSET = 0x4000
TITLE = b"MY BABY A.I."
CODE = b"MBAI"


def crc16(data: bytes) -> int:
    crc = 0xffff
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ (0xa001 if crc & 1 else 0)
    return crc


def u32(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 4], "little")


def create_plain(original: bytes) -> bytes:
    if len(original) < 0x8000:
        raise ValueError("Incomplete NDS cartridge image")
    if original[:12] != TITLE or original[12:16] != CODE:
        raise ValueError("Not My Baby A.I.; refusing to rewrite another ROM")
    if original[0x12] != 0:
        raise ValueError("First create the DS-only unit-code ROM")
    if crc16(original[:0x15e]) != int.from_bytes(original[0x15e:0x160], "little"):
        raise ValueError("Input header CRC mismatch")

    arm9, size9 = u32(original, 0x20), u32(original, 0x2c)
    arm7, size7 = u32(original, 0x30), u32(original, 0x3c)
    if arm9 != ARM9_RETAIL_OFFSET or size9 <= 0x800 or arm9 + size9 > len(original):
        raise ValueError("Unknown ARM9 layout")
    if arm7 < 0x8000 or not size7 or arm7 + size7 > len(original):
        raise ValueError("Invalid ARM7 range")
    if ARM9_PLAIN_OFFSET + size9 > arm7:
        raise ValueError("ARM9 relocation would overwrite ARM7")
    if any(original[arm9:arm9 + 0x800]):
        raise ValueError("Expected blank secure prefix; refusing non-homebrew input")
    # All referenced file sections must be above ARM7; do not move or erase
    # the filesystem, overlays, banner or static resources.
    for field in (0x40, 0x48, 0x50, 0x58, 0x68):
        at = u32(original, field)
        if at and at < arm7:
            raise ValueError("Content offset would overlap relocated ARM9")

    result = bytearray(original)
    result[ARM9_PLAIN_OFFSET:ARM9_PLAIN_OFFSET + size9] = original[arm9:arm9 + size9]
    result[0x20:0x24] = ARM9_PLAIN_OFFSET.to_bytes(4, "little")
    result[0x84:0x88] = ARM9_PLAIN_OFFSET.to_bytes(4, "little")
    result[0x6c:0x6e] = b"\x00\x00"  # No secure area in the homebrew layout.
    result[0x15e:0x160] = crc16(result[:0x15e]).to_bytes(2, "little")

    assert result[ARM9_PLAIN_OFFSET:ARM9_PLAIN_OFFSET + size9] == original[arm9:arm9 + size9]
    assert result[0x200:ARM9_PLAIN_OFFSET] == original[0x200:ARM9_PLAIN_OFFSET]
    assert result[arm7:] == original[arm7:]
    assert len(result) == len(original)
    assert crc16(result[:0x15e]) == u32(result[0x15e:0x160] + b"\x00\x00", 0)
    return bytes(result)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error("Input and output must be different files")
    result = create_plain(args.source.read_bytes())
    args.output.write_bytes(result)
    print(f"Plain homebrew ROM {args.output} ({len(result):,} bytes; "
          f"ARM9 at {u32(result, 0x20):#x}; DS-only unit code)")


if __name__ == "__main__":
    main()
