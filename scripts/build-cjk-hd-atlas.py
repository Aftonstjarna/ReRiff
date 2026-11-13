"""Build paged 64/96px native atlases with unchanged 32px layout metrics."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('cjk_base', ROOT/'scripts/build-cjk-atlas.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def bake(source, output, region='jp', pixels=64, characters=None, page_size=8192):
    if pixels not in (64, 96) or page_size not in (256, 512, 8192):
        raise ValueError('Unsupported quality tier or page size')
    expected = json.loads((ROOT/'packaging/fonts/noto-cjk.json').read_text())['files'][0]
    if source.stat().st_size != expected['size'] or hashlib.sha256(source.read_bytes()).hexdigest() != expected['sha256']:
        raise ValueError('Noto input does not match pinned source')
    face = base.REGIONS.index(region)
    with TTFont(source, fontNumber=face, lazy=True) as metadata:
        cmap = metadata.getBestCmap()
        codes = sorted(cmap if characters is None else set(map(ord, characters)))
        if any(code not in cmap for code in codes):
            raise ValueError('Requested character absent from pinned face')
    codes = [c for c in codes if c >= 32 and not 0xd800 <= c <= 0xdfff]
    if not codes:
        raise ValueError('No characters selected')
    kwargs = dict(index=face, layout_engine=ImageFont.Layout.BASIC)
    font = ImageFont.truetype(str(source), pixels, **kwargs)
    design = ImageFont.truetype(str(source), 32, **kwargs)
    ascent, descent = design.getmetrics()
    records, page, x, y, row = [], 0, 2, 2, 0
    for code in codes:
        left, top, right, bottom = font.getbbox(chr(code), anchor='ls')
        w, h = right-left, bottom-top
        if w+4 > page_size or h+4 > page_size:
            raise ValueError('Glyph exceeds page size')
        if x+w+2 > page_size:
            x, y, row = 2, y+row+4, 0
        if y+h+2 > page_size:
            page, x, y, row = page+1, 2, 2, 0
        dl, dt, dr, db = design.getbbox(chr(code), anchor='ls')
        records.append((code, page, x, y, w, h, dl, ascent+dt,
                        float(design.getlength(chr(code))), dr-dl, db-dt))
        x += w+4
        row = max(row, h)
    if page >= 8:
        raise ValueError('Atlas exceeds eight native pages')
    label = region+str(pixels)
    target = output/'graphics/fonts/reriff/cjk'/label
    for number in range(page+1):
        alpha = Image.new('L', (page_size, page_size), 0)
        draw = ImageDraw.Draw(alpha)
        for code, p, x, y, *_ in records:
            if p != number:
                continue
            left, top, _, _ = font.getbbox(chr(code), anchor='ls')
            draw.text((x-left, y-top), chr(code), font=font,
                      anchor='ls', fill=255, stroke_width=0)
        base.write_texture(target/f'p{number:03}', f'reriff_cjk_{label}_p{number:03}', alpha)
    # V2: 12 DWORD header, then code/page + nine floats per record. Image
    # dimensions use the high-detail font; all layout metrics use the exact
    # same 32px face as V1, including bearings, advance and visible bounds.
    data = struct.pack('<12I', 0x46435252, 2, len(records), page_size, page_size,
                       pixels, ascent, descent, page+1, 44, 32, 0)
    data += b''.join(struct.pack('<2I9f', *record) for record in records)
    (target/'glyphs.rrcf').write_bytes(data)
    summary = dict(region=region, pixels=pixels, pages=page+1, page_size=page_size,
                   codepoints=len(codes), design_pixels=32,
                   source_sha256=expected['sha256'], metrics_sha256=hashlib.sha256(data).hexdigest())
    (target/'build.json').write_text(json.dumps(summary, indent=2)+'\n')
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT/'build/noto-cjk/NotoSansCJK-Regular.ttc')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--region', choices=base.REGIONS, default='jp')
    parser.add_argument('--pixels', type=int, choices=(64, 96), required=True)
    args = parser.parse_args()
    print(json.dumps(bake(args.source, args.output, args.region, args.pixels)), flush=True)
