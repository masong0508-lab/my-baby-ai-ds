#!/usr/bin/env python3
"""Package a DS-only compatibility ROM from the exact built program.

Nintendo DS cartridge header byte 0x12: 0 = DS-only, 2 = DS+DSi.
This game uses only original DS hardware. Some legacy emulator builds reject
DSi-enhanced-labelled homebrew. We change that single byte plus the required
header CRC16, leaving all ARM9/ARM7 program bytes unchanged.
This does not fix unrelated emulator/JIT bugs or guarantee runtime operation.
"""
from __future__ import annotations
import argparse
from pathlib import Path

def crc16(data: bytes) -> int:
    crc = 0xffff
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xa001 if (crc & 1) else 0)
    return crc

def u32(b: bytes, offset: int) -> int:
    return int.from_bytes(b[offset:offset+4], "little")

def patch(original: bytes) -> bytes:
    b = bytearray(original)
    if len(b) < 0x4000:
        raise ValueError("Not an NDS cartridge ROM")
    arm9_offset, arm9_size = u32(b, 0x20), u32(b, 0x2c)
    arm7_offset, arm7_size = u32(b, 0x30), u32(b, 0x3c)
    for offset, size, name in ((arm9_offset, arm9_size, "ARM9"),
                               (arm7_offset, arm7_size, "ARM7")):
        if offset < 0x200 or size == 0 or offset+size > len(b):
            raise ValueError("Invalid "+name+" code range")
    if b[0x12] not in (0, 2):
        raise ValueError("Unexpected unit code "+str(b[0x12]))
    if crc16(b[:0x15e]) != int.from_bytes(b[0x15e:0x160], "little"):
        raise ValueError("Input DS header checksum mismatch")
    b[0x12] = 0  # Original Nintendo DS unit code.
    b[0x15e:0x160] = crc16(b[:0x15e]).to_bytes(2, "little")
    assert crc16(b[:0x15e]) == int.from_bytes(b[0x15e:0x160], "little")
    assert b[arm9_offset:arm9_offset+arm9_size] == original[arm9_offset:arm9_offset+arm9_size]
    assert b[arm7_offset:arm7_offset+arm7_size] == original[arm7_offset:arm7_offset+arm7_size]
    return bytes(b)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error("Source and output must differ")
    before = args.source.read_bytes()
    result = patch(before)
    args.output.write_bytes(result)
    print(f"DS-only ROM: {args.output} ({len(result):,} bytes, "
          f"unitcode={result[0x12]}, header_crc={result[0x15e:0x160].hex()})")
