#!/usr/bin/env python3
"""Make safe, bannered My Baby A.I. legacy-DraStic test ROMs.

This is packaging original homebrew, not decrypting a retail cartridge.
The input must be an unencrypted, CRC-valid DS-only My Baby A.I. toolchain
ROM with ARM9 at 0x4000 and the first 0x800 code bytes blank. ARM7 and
NitroFS sections are never moved. Existing banner must be valid, if present.
"""
from __future__ import annotations
import argparse
from pathlib import Path
from nds_plain_homebrew import create_plain, crc16, u32, TITLE, CODE

BANNER_LEN = 0x840
COLORS = (
    (0,0,0), (3,5,12), (8,13,22), (18,25,26),
    (30,23,20), (27,17,14), (31,31,29), (4,6,10),
    (12,8,6), (29,14,17), (12,26,23), (7,18,18),
    (31,25,13), (15,31,18), (23,24,24), (8,25,28),
)

def icon_pixels() -> list[list[int]]:
    px = [[1 for _ in range(32)] for _ in range(32)]
    def rect(x1:int,y1:int,x2:int,y2:int,c:int)->None:
        for y in range(max(y1,0),min(y2,32)):
            for x in range(max(x1,0),min(x2,32)):
                px[y][x]=c
    def oval(cx:int,cy:int,rx:int,ry:int,c:int)->None:
        for y in range(max(cy-ry,0),min(cy+ry+1,32)):
            for x in range(max(cx-rx,0),min(cx+rx+1,32)):
                if (x-cx)**2*ry**2+(y-cy)**2*rx**2<=rx**2*ry**2:
                    px[y][x]=c
    rect(2,2,30,30,2); rect(4,4,28,28,15)
    rect(4,23,28,28,2)
    oval(16,30,12,11,11); oval(16,28,11,9,10)
    oval(16,15,10,10,5); oval(16,13,10,9,4)
    oval(16,8,9,4,8); rect(8,8,12,10,8); rect(13,5,20,9,8)
    oval(12,15,2,3,6); oval(20,15,2,3,6)
    oval(12,16,1,2,7); oval(20,16,1,2,7)
    rect(9,20,11,22,9); rect(21,20,23,22,9)
    rect(14,20,19,21,7); rect(15,21,18,22,9)
    rect(16,24,18,25,12); rect(22,5,27,9,13)
    rect(23,6,26,8,6)
    return px

def build_banner() -> bytes:
    data=bytearray(BANNER_LEN)
    data[:2]=(1).to_bytes(2,"little")
    px=icon_pixels()
    for ty in range(4):
        for tx in range(4):
            for y in range(8):
                for x in range(0,8,2):
                    at=0x20+(ty*4+tx)*32+y*4+x//2
                    data[at]=px[ty*8+y][tx*8+x]|(px[ty*8+y][tx*8+x+1]<<4)
    for i,(r,g,b) in enumerate(COLORS):
        data[0x220+i*2:0x222+i*2]=(r|g<<5|b<<10).to_bytes(2,"little")
    title="My Baby A.I.\nThe Last Nursery".encode("utf-16le")
    for language in range(6):
        off=0x240+language*0x100
        data[off:off+len(title)]=title
    data[2:4]=crc16(data[0x20:]).to_bytes(2,"little")
    return bytes(data)

def make_rom(original:bytes,layout:str="classic")->bytes:
    if layout not in ("classic","plain","standard"):
        raise ValueError("Unknown legacy ROM layout")
    if len(original)<0x8000 or original[:12]!=TITLE or original[12:16]!=CODE:
        raise ValueError("Not My Baby A.I.")
    if original[0x12]!=0 or u32(original,0x20)!=0x4000 or u32(original,0x84)!=0x4000:
        raise ValueError("Expected original DS-only toolchain ROM")
    if crc16(original[:0x15e])!=int.from_bytes(original[0x15e:0x160],"little"):
        raise ValueError("Invalid source header CRC")
    off9,size9,off7,size7=(u32(original,pos) for pos in (0x20,0x2c,0x30,0x3c))
    if size9<=0x800 or off9+size9>off7 or off7+size7>len(original) or not size7:
        raise ValueError("Original code regions overlap or exceed the ROM")
    if any(original[0x4000:0x4800]):
        raise ValueError("Nonblank secure area: refusing non-homebrew input")
    for pos in (0x40,0x48,0x50,0x58):
        at,size=u32(original,pos),u32(original,pos+4)
        if size and (at<off7+size7 or at+size>len(original)):
            raise ValueError("Files or overlays overlap ARM7")
    existing=u32(original,0x68)
    if existing and (
        existing<off7+size7 or existing+BANNER_LEN>len(original) or
        int.from_bytes(original[existing:existing+2],"little")!=1 or
        crc16(original[existing+0x20:existing+BANNER_LEN])!=
        int.from_bytes(original[existing+2:existing+4],"little")
    ):
        raise ValueError("Existing banner is corrupt")
    if u32(original,0x80)>len(original):
        raise ValueError("Used ROM size exceeds file")

    # Existing plain converter copies the compiled ARM9 unchanged to 0x1000.
    # Our classic layout extends that to the older canonical 0x200 offset.
    out=bytearray(original if layout=="standard" else create_plain(original))
    target={"classic":0x200,"plain":0x1000,"standard":0x4000}[layout]
    if target+size9>off7:
        raise ValueError("Relocated ARM9 overlaps ARM7")
    if layout=="classic":
        out[target:target+size9]=original[off9:off9+size9]
    out[0x20:0x24]=target.to_bytes(4,"little")
    out[0x84:0x88]=(0x4000 if layout=="standard" else 0x200).to_bytes(4,"little")
    out[0x6c:0x6e]=b"\0\0" # unencrypted homebrew has no secure CRC
    banner=build_banner()
    banner_offset=(len(out)+0x1FF)&~0x1FF
    out.extend(b"\0"*(banner_offset-len(out)))
    out.extend(banner)
    used=len(out)
    out.extend(b"\0"*((-len(out))&0x1FF))
    if len(out)>(1 << (17+original[0x14])):
        raise ValueError("Banner exceeds original cart capacity")
    out[0x68:0x6c]=banner_offset.to_bytes(4,"little")
    out[0x80:0x84]=used.to_bytes(4,"little")
    out[0x15e:0x160]=crc16(out[:0x15e]).to_bytes(2,"little")
    assert out[target:target+size9]==original[off9:off9+size9]
    assert out[off7:len(original)]==original[off7:]
    assert crc16(out[banner_offset+0x20:banner_offset+BANNER_LEN])==int.from_bytes(banner[2:4],"little")
    assert crc16(out[:0x15e])==int.from_bytes(out[0x15e:0x160],"little")
    return bytes(out)

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source",type=Path)
    parser.add_argument("output",type=Path)
    parser.add_argument("--layout",choices=("classic","plain","standard"),default="classic")
    args=parser.parse_args()
    if args.source.resolve()==args.output.resolve():
        parser.error("Input and output must differ")
    data=make_rom(args.source.read_bytes(),args.layout)
    args.output.write_bytes(data)
    print(f"{args.output}: {len(data):,} bytes, ARM9={u32(data,0x20):#x}, "
          f"header={u32(data,0x84):#x}, banner={u32(data,0x68):#x}")

if __name__=="__main__":
    main()
