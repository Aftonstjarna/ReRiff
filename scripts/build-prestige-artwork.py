"""Encode supplied native-size artwork with circular runtime transparency.

Uses the original TABT v4/DDS texture container. The 64x64 circular source is
already square. Authored alpha is preserved. Legacy RGB inputs receive a
circular alpha channel because their square corners otherwise stay opaque.
"""
import argparse
import hashlib
import io
import math
from pathlib import Path
import struct
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]

def build(source, destination):
    with Image.open(source) as source_image:
        if source_image.size != (64, 64):
            raise ValueError('Expected the reviewed square 64x64 achievement source')
        image = source_image.convert('RGBA')
        # Preserve transparency supplied by the artist. Apply the legacy
        # circular clip only when the source has no alpha/transparency.
        if 'A' not in source_image.getbands() and 'transparency' not in source_image.info:
            alpha = image.getchannel('A')
            for y in range(64):
                for x in range(64):
                    distance = math.hypot(x + .5 - 32, y + .5 - 32)
                    coverage = max(0., min(1., 32.5 - distance))
                    alpha.putpixel((x, y), round(alpha.getpixel((x, y))*coverage))
            image.putalpha(alpha)
        dds = io.BytesIO()
        image.save(dds, format='DDS', pixel_format='DXT3')
    payload = bytearray(dds.getvalue())
    # BC2 has an explicit little-endian 4-bit alpha value per pixel. Encode
    # those nibbles directly: the Pillow 12 encoder can scramble partial-alpha
    # samples, although fully opaque/transparent fixtures appear correct.
    # Keep the exporter's compressed RGB blocks unchanged.
    alpha = image.getchannel('A')
    for by in range(16):
        for bx in range(16):
            bits = 0
            for dy in range(4):
                for dx in range(4):
                    value = (alpha.getpixel((bx*4+dx, by*4+dy))*15+127)//255
                    bits |= value << (4*(dy*4+dx))
            struct.pack_into('<Q', payload, 128+(by*16+bx)*16, bits)
    payload = bytes(payload)
    if payload[:4] != b'DDS ' or payload[84:88] != b'DXT3':
        raise ValueError('Unexpected native texture encoding')
    name = b'first_prestige\0'
    offset = (52 + len(name) + 31) & ~31
    pool = name + b'\0'*(offset - 52 - len(name))
    tabt = struct.pack('<7I', 0x54424154, 0xffee, 4, offset, 1, len(pool), len(payload))
    tabt += struct.pack('<6I', 0, 0, len(payload), 64, 64, 2) + pool + payload
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(tabt)
    return hashlib.sha256(tabt).hexdigest()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT/'images/reriff-prestige-achievement.png')
    parser.add_argument('--output', type=Path, default=ROOT/'packaging/core/graphics/reriff/achievements/batchtextures.brres')
    args = parser.parse_args()
    print(build(args.source, args.output), args.output)
