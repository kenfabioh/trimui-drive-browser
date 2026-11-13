#!/usr/bin/env python3
"""Validate the approved D na nuvem icon without regenerating older artwork."""
from pathlib import Path
import struct

data = Path('app/icon.png').read_bytes()
assert data[:8] == b'\x89PNG\r\n\x1a\n', 'Expected PNG icon'
width, height, depth, color = struct.unpack_from('!IIBB', data, 16)
assert width == height == 300 and depth == 8 and color == 6
print(f'Icone aprovado D na nuvem: {width}x{height}, RGBA. Arte preservada.')
