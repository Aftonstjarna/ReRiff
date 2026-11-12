"""Bake pinned Noto CJK into owned native textures and Unicode metrics.

Build-only dependencies: packaging/fonts/build-requirements.txt.
This does not install assets or change the original game archives.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
REGIONS = ('jp', 'kr', 'sc', 'tc', 'hk')


def write_texture(target, texture_name, alpha):
    width, height = alpha.size
    rgba = Image.new('RGBA', alpha.size, (255, 255, 255, 0))
    rgba.putalpha(alpha)
    dds = io.BytesIO()
    # DXT3 is a D3D9-native BC2 format with explicit antialiased alpha.
    rgba.save(dds, format='DDS', pixel_format='DXT3')
    payload = dds.getvalue()
    if payload[:4] != b'DDS ' or payload[84:88] != b'DXT3':
        raise ValueError('Unexpected DDS encoder output')
    name = texture_name.encode('ascii')+b'\0'
    payload_offset = (28+24+len(name)+31) & ~31
    # Padding belongs to the name pool; native payload offsets remain exact.
    pool = name+b'\0'*(payload_offset-52-len(name))
    tabt = struct.pack('<7I', 0x54424154, 0xffee, 4, payload_offset, 1,
                       len(pool), len(payload))
    tabt += struct.pack('<6I', 0, 0, len(payload), width, height, 2)+pool+payload
    target.mkdir(parents=True, exist_ok=True)
    (target/'batchtextures.brres').write_bytes(tabt)
    return tabt

def bake(source, output, region, size=32, characters=None):
    manifest = json.loads((ROOT/'packaging/fonts/noto-cjk.json').read_text())
    expected = manifest['files'][0]
    if (source.stat().st_size != expected['size'] or
            hashlib.sha256(source.read_bytes()).hexdigest() != expected['sha256']):
        raise ValueError('Noto input does not match pinned source')
    face = REGIONS.index(region)
    with TTFont(source, fontNumber=face, lazy=True) as metadata:
        cmap = metadata.getBestCmap()
        codes = sorted(cmap if characters is None else set(map(ord, characters)))
        if any(code not in cmap for code in codes):
            raise ValueError('Requested character is absent from the pinned font')
    codes = [code for code in codes if code >= 32 and not 0xd800 <= code <= 0xdfff]
    if not codes:
        raise ValueError('No printable characters selected')
    font = ImageFont.truetype(str(source), size, index=face,
                              layout_engine=ImageFont.Layout.BASIC)
    # Baseline anchored metrics retain negative bearings and descenders.
    # Regional fonts have different glyph designs, not different code mappings.
    ascent, descent = font.getmetrics()
    width = 512 if len(codes) <= 256 else 8192
    positions, x, y, row_height = [], 2, 2, 0
    for code in codes:
        left, top, right, bottom = font.getbbox(chr(code), anchor='ls')
        w, h = right-left, bottom-top
        if w+4 > width or h+4 > 8192:
            raise ValueError('Glyph exceeds atlas limits')
        if x+w+2 > width:
            x, y, row_height = 2, y+row_height+4, 0
        positions.append((code, x, y, w, h, left, ascent+top,
                          float(font.getlength(chr(code)))))
        x += w+4
        row_height = max(row_height, h)
    required = y+row_height+2
    height = 1 << (max(required, 4)-1).bit_length()
    if height > 8192:
        raise ValueError('Atlas exceeds the 8192 pixel build limit')
    alpha = Image.new('L', (width, height), 0)
    draw = ImageDraw.Draw(alpha)
    records = bytearray()
    for code, x, y, w, h, left, offset_y, advance in positions:
        # Place the bounding box exactly at the packed coordinates.
        draw.text((x-left, y-offset_y+ascent), chr(code), font=font,
                  fill=255, anchor='ls', stroke_width=0)
        records.extend(struct.pack('<I7f', code, x, y, w, h, left, offset_y, advance))
    target = output/'graphics/fonts/reriff/cjk'/region
    tabt = write_texture(target, 'reriff_cjk_'+region, alpha)
    # RRCF v1, eight DWORD header then sorted <codepoint, 7 float> records.
    # Keep full codepoints, including supplementary characters, in owned data.
    # Native WORD glyph records must be made separately after pair handling.
    metrics = struct.pack('<8I', 0x46435252, 1, len(codes), width, height,
                          size, ascent, descent)+records
    (target/'glyphs.rrcf').write_bytes(metrics)
    summary = dict(region=region, family=manifest['regions'][region],
                   source_sha256=expected['sha256'], codepoints=len(codes),
                   supplementary=sum(code > 65535 for code in codes),
                   pixels=size, width=width, height=height,
                   texture_sha256=hashlib.sha256(tabt).hexdigest(),
                   metrics_sha256=hashlib.sha256(metrics).hexdigest())
    (target/'build.json').write_text(json.dumps(summary, indent=2)+'\n')
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path,
                        default=ROOT/'build/noto-cjk/NotoSansCJK-Regular.ttc')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--region', choices=(*REGIONS, 'all'), default='all')
    parser.add_argument('--characters', help='Limit coverage for a small development probe')
    args = parser.parse_args()
    for region in REGIONS if args.region == 'all' else (args.region,):
        print(json.dumps(bake(args.source, args.output, region,
                              characters=args.characters)), flush=True)
