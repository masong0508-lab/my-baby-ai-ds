#!/usr/bin/env python3
"""Synthetic DS header tests; no SDK, download, runtime or ROM assets needed."""
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from nds_legacy_header import crc16, patch as ds_only_patch
from nds_rom_identity import CODE, TITLE, patch_rom


def source_rom() -> bytes:
    r = bytearray(0x6000)
    r[:12] = b"HOMEBREW".ljust(12, bytes([0]))
    r[0x0c:0x10] = b"TEST"
    r[0x12] = 2  # DSi-compatible input, patched later for DS-only.
    struct.pack_into("<IIII", r, 0x20, 0x4000, 0x02000000,
                     0x02000000, 0x100)
    struct.pack_into("<IIII", r, 0x30, 0x4800, 0x02380000,
                     0x02380000, 0x140)
    for i in range(0x100):
        r[0x4000+i] = i & 255
    for i in range(0x140):
        r[0x4800+i] = (i*3) & 255
    r[0x15e:0x160] = crc16(r[:0x15e]).to_bytes(2, "little")
    return bytes(r)


def expect_invalid(value: bytes) -> None:
    try:
        patch_rom(value)
    except ValueError:
        return
    raise AssertionError("Malformed ROM accepted")


def main() -> None:
    raw = source_rom()
    fixed = patch_rom(raw)
    assert fixed[:12] == TITLE
    assert fixed[12:16] == CODE
    assert len(fixed) == len(raw)
    assert fixed[0x160:] == raw[0x160:]
    assert fixed[0x4000:0x4100] == raw[0x4000:0x4100]
    assert fixed[0x4800:0x4940] == raw[0x4800:0x4940]
    assert crc16(fixed[:0x15e]) == int.from_bytes(
        fixed[0x15e:0x160], "little")
    assert patch_rom(fixed) == fixed  # Safe for repeated 'make' commands.
    legacy = ds_only_patch(fixed)
    assert legacy[0x12] == 0
    assert legacy[:16] == fixed[:16]
    assert legacy[0x160:] == fixed[0x160:]
    assert crc16(legacy[:0x15e]) == int.from_bytes(
        legacy[0x15e:0x160], "little")
    expect_invalid(bytes([0]) * 14)
    bad = bytearray(raw)
    bad[0] ^= 1
    expect_invalid(bytes(bad))
    bad = bytearray(raw)
    struct.pack_into("<I", bad, 0x2c, 0x4000)
    bad[0x15e:0x160] = crc16(bad[:0x15e]).to_bytes(2, "little")
    expect_invalid(bytes(bad))
    print("PASS: DS identity, header CRC, ARM invariance, DS-only conversion")


if __name__ == "__main__":
    main()
