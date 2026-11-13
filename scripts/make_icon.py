#!/usr/bin/env python3
"""Validate the approved artwork against the CrossMix 1.3.0 launcher layout."""
from pathlib import Path
import struct
import zlib


def check_icon(data):
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'Expected PNG icon'
    width, height, depth, color, compression, filtering, interlace = struct.unpack_from('!IIBBBBB', data, 16)
    assert (width,height,depth,color,compression,filtering,interlace)==(300,300,8,6,0,0,0)
    compressed=bytearray()
    offset=8
    while offset<len(data):
        length=struct.unpack_from('!I',data,offset)[0]
        kind=data[offset+4:offset+8]
        body=data[offset+8:offset+8+length]
        crc=struct.unpack_from('!I',data,offset+8+length)[0]
        assert zlib.crc32(kind+body)==crc, 'PNG chunk CRC mismatch'
        if kind==b'IDAT': compressed.extend(body)
        offset+=length+12
        if kind==b'IEND': break
    raw=zlib.decompress(compressed)
    stride=width*4
    assert len(raw)==height*(stride+1)
    previous=bytearray(stride)
    minx,miny,maxx,maxy=width,height,0,0
    for y in range(height):
        pos=y*(stride+1)
        filter_type=raw[pos]
        row=bytearray(raw[pos+1:pos+1+stride])
        assert 0<=filter_type<=4
        for i in range(stride):
            left=row[i-4] if i>=4 else 0
            above=previous[i]
            upper_left=previous[i-4] if i>=4 else 0
            predictor=0
            if filter_type==1: predictor=left
            if filter_type==2: predictor=above
            if filter_type==3: predictor=(left+above)//2
            if filter_type==4:
                p=left+above-upper_left
                distances=(abs(p-left),abs(p-above),abs(p-upper_left))
                predictor=(left,above,upper_left)[distances.index(min(distances))]
            row[i]=(row[i]+predictor)&255
        for x in range(width):
            if row[x*4+3]:
                minx,miny,maxx,maxy=min(minx,x),min(miny,y),max(maxx,x+1),max(maxy,y+1)
        previous=row
    bounds=(minx,miny,maxx,maxy)
    # User Guide and PortMaster use (65,25)-(235,195) on a 300x300 canvas.
    # Antialiased edge pixels may disappear on downsampling; keep the whole
    # illustration within that box and reserve all space below it for the title.
    assert 65<=minx and 25<=miny and maxx<=235 and maxy<=195, f'Artwork overlaps launcher title area: {bounds}'
    assert 165<=maxx-minx<=170 and 165<=maxy-miny<=170, f'Unexpected artwork size: {bounds}'
    return bounds


if __name__=='__main__':
    bounds=check_icon(Path('app/icon.png').read_bytes())
    print('PASS icone RGBA 300x300; arte dentro da area CrossMix 170x170:',bounds)
