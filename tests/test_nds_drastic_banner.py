#!/usr/bin/env python3
"""Verify authentic version-1 NDS banner and safe classic-homebrew ROMs."""
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from nds_drastic_banner import BANNER_LEN, crc16, make_rom, u32, build_banner, TITLE, CODE

def fixture(existing_banner=False)->bytes:
    data=bytearray(0xb000)
    data[:12]=TITLE;data[12:16]=CODE
    data[0x10:0x12]=b"00";data[0x12]=0;data[0x14]=2
    for off,value in (
        (0x20,0x4000),(0x24,0x02000800),(0x28,0x02000000),(0x2c,0x1c00),
        (0x30,0x8000),(0x34,0x02380000),(0x38,0x02380000),(0x3c,0x0800),
        (0x40,0x9000),(0x44,9),(0x48,0x9200),(0x4c,0),
        (0x80,0x9400),(0x84,0x4000)
    ):
        struct.pack_into("<I",data,off,value)
    for i in range(0x800,0x1c00):
        data[0x4000+i]=(i*53+3)&255
    for i in range(0x800):
        data[0x8000+i]=(i*17+7)&255
    data[0x9000:0x9009]=b"\x04\x00\x00\xf0\x00\x00\x00\x00\x00"
    if existing_banner:
        banner=build_banner()
        data[0x9800:0x9800+BANNER_LEN]=banner
        struct.pack_into("<I",data,0x68,0x9800)
        struct.pack_into("<I",data,0x80,0x9800+BANNER_LEN)
    data[0x15e:0x160]=crc16(data[:0x15e]).to_bytes(2,"little")
    return bytes(data)

def reject(data:bytes,layout="classic"):
    try:
        make_rom(data,layout)
    except ValueError:
        return
    raise AssertionError("Malformed or unsafe ROM accepted")

def check(original:bytes,variant:bytes,layout:str):
    target={"classic":0x200,"plain":0x1000,"standard":0x4000}[layout]
    assert u32(variant,0x20)==target
    assert u32(variant,0x84)==(0x4000 if layout=="standard" else 0x200)
    assert variant[:16]==original[:16]
    assert variant[0x12]==0 and variant[0x6c:0x6e]==b"\0\0"
    size9=u32(original,0x2c)
    assert variant[target:target+size9]==original[0x4000:0x4000+size9]
    arm7=u32(original,0x30)
    assert variant[arm7:len(original)]==original[arm7:]
    for field in (0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,0x40,0x44,
                  0x48,0x4c,0x50,0x54,0x58,0x5c):
        assert u32(variant,field)==u32(original,field),hex(field)
    banner=u32(variant,0x68)
    assert banner>=len(original) and not banner%0x200
    assert variant[banner:banner+BANNER_LEN]==build_banner()
    assert variant[banner:banner+2]==b"\x01\x00"
    assert crc16(variant[banner+0x20:banner+BANNER_LEN])==int.from_bytes(variant[banner+2:banner+4],"little")
    title=variant[banner+0x340:banner+0x440].decode("utf-16le").split("\0")[0]
    assert title=="My Baby A.I.\nThe Last Nursery",repr(title)
    assert crc16(variant[:0x15e])==int.from_bytes(variant[0x15e:0x160],"little")
    assert variant[0x15c:0x15e]==original[0x15c:0x15e]
    assert u32(variant,0x80)>=banner+BANNER_LEN

def main():
    for existing in (False,True):
        original=fixture(existing)
        for layout in ("classic","plain","standard"):
            result=make_rom(original,layout)
            check(original,result,layout)
            assert result==make_rom(original,layout)
    corrupted=bytearray(fixture())
    corrupted[0]^=0x10; reject(corrupted)
    corrupted=bytearray(fixture())
    corrupted[0x12]=2; reject(corrupted)
    corrupted=bytearray(fixture())
    corrupted[0x4000]=0x44
    corrupted[0x15e:0x160]=crc16(corrupted[:0x15e]).to_bytes(2,"little")
    reject(corrupted)
    corrupted=bytearray(fixture())
    struct.pack_into("<I",corrupted,0x20,0x1000)
    corrupted[0x15e:0x160]=crc16(corrupted[:0x15e]).to_bytes(2,"little")
    reject(corrupted)
    corrupted=bytearray(fixture())
    struct.pack_into("<I",corrupted,0x2c,0x5000)
    corrupted[0x15e:0x160]=crc16(corrupted[:0x15e]).to_bytes(2,"little")
    reject(corrupted)
    corrupted=bytearray(fixture())
    struct.pack_into("<I",corrupted,0x68,0x8800)
    corrupted[0x15e:0x160]=crc16(corrupted[:0x15e]).to_bytes(2,"little")
    reject(corrupted)
    reject(fixture()[:300])
    reject(fixture(),"invalid")
    print("PASS: NDS icon+UTF-16 title, banner CRC, legacy header, ARM9 mapping and refusal checks")

if __name__=="__main__":
    main()
