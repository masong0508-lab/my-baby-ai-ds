#!/usr/bin/env python3
"""Reproduce and verify a retail-layout-to-homebrew NDS packaging repair."""
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from nds_plain_homebrew import (
    ARM9_PLAIN_OFFSET, ARM9_RETAIL_OFFSET, crc16, u32, create_plain
)


def fixture() -> bytes:
    r = bytearray(0xc000)
    r[:12] = b"MY BABY A.I."
    r[12:16] = b"MBAI"
    r[0x12] = 0
    struct.pack_into("<I", r, 0x20, ARM9_RETAIL_OFFSET)
    struct.pack_into("<I", r, 0x2c, 0x3700)
    struct.pack_into("<I", r, 0x30, 0x8000)
    struct.pack_into("<I", r, 0x3c, 0x1000)
    struct.pack_into("<I", r, 0x40, 0x9000)
    struct.pack_into("<I", r, 0x48, 0x9200)
    struct.pack_into("<I", r, 0x68, 0x9800)
    struct.pack_into("<I", r, 0x84, 0x4000)
    r[0x200:0x210] = b"EXTENDED HEADER!"  # all 16 bytes
    r[0xfff] = 13
    # Blank 0x4000..0x47ff; executable begins after the reserved prefix.
    for i in range(0x800, 0x3700):
        r[ARM9_RETAIL_OFFSET + i] = (i * 67 + 23) % 251 + 1
    for i in range(0x8000, len(r)):
        r[i] = (i * 13 + 7) % 251 + 1
    r[0x15e:0x160] = crc16(r[:0x15e]).to_bytes(2, "little")
    return bytes(r)


def reject(data: bytes) -> None:
    try:
        create_plain(data)
    except ValueError:
        return
    raise AssertionError("Unsafe or invalid NDS layout was accepted")


def main() -> None:
    src = fixture()
    out = create_plain(src)
    assert out != src and len(out) == len(src)
    assert out[:16] == src[:16] and out[0x12] == 0
    assert u32(out, 0x20) == ARM9_PLAIN_OFFSET < 0x4000
    assert u32(out, 0x84) == 0x1000
    assert out[0x6c:0x6e] == b"\x00\x00"
    assert out[0x1000:0x4700] == src[0x4000:0x7700]
    assert out[0x1800:0x1810] == src[0x4800:0x4810]
    assert out[0x200:0x1000] == src[0x200:0x1000]
    assert out[0x8000:] == src[0x8000:]
    assert crc16(out[:0x15e]) == int.from_bytes(out[0x15e:0x160], "little")
    assert all(u32(out, field) == u32(src, field) for field in
               (0x2c, 0x24, 0x28, 0x30, 0x34, 0x38, 0x3c, 0x40, 0x48, 0x68))
    reject(src[:400])
    reject(out)  # Not a tool for repeatedly mutating new layouts.
    v = bytearray(src)
    v[0x12] = 2
    reject(v)
    v = bytearray(src)
    v[0x0c] = 0
    reject(v)
    v = bytearray(src)
    v[0x160] = 1
    v[0x4000] = 2  # A nonblank secure area must not be claimed as plain.
    v[0x15e:0x160] = crc16(v[:0x15e]).to_bytes(2, "little")
    reject(v)
    v = bytearray(src)
    struct.pack_into("<I", v, 0x2c, 0x7000)  # Overlaps ARM7 after relocation.
    v[0x15e:0x160] = crc16(v[:0x15e]).to_bytes(2, "little")
    reject(v)
    v = bytearray(src)
    struct.pack_into("<I", v, 0x40, 0x3000)  # No filesystem relocation.
    v[0x15e:0x160] = crc16(v[:0x15e]).to_bytes(2, "little")
    reject(v)
    v = bytearray(src)
    v[0] ^= 1
    reject(v)
    print("PASS: plain homebrew ARM9 boot layout, CRC, ROM invariants and safety guards")


if __name__ == "__main__":
    main()
