"""Bake a separately downloadable Noto Sans Latin/European font pack.

Uses the existing native font formats and shares one installed font VFS with CJK.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import zipfile
from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module

base = load('latin_base', ROOT/'scripts/build-cjk-atlas.py')
archive = load('latin_archive', ROOT/'packaging/native_archive.py')

def bake(output, pixels, characters=None):
    pin = json.loads((ROOT/'packaging/fonts/noto-latin.json').read_text())
    source = ROOT/'build/noto-latin'/Path(pin['files'][0]['path']).name
    if hashlib.sha256(source.read_bytes()).hexdigest() != pin['files'][0]['sha256']:
        raise ValueError('Noto Sans source checksum mismatch')
    with TTFont(source, lazy=True) as font:
        supported = set(font.getBestCmap())
    codes = sorted((supported & set(range(0xa0, 0x250))) if characters is None else set(map(ord, characters)))
    if not codes or any(c not in supported for c in codes):
        raise ValueError('Unsupported Latin character')
    # Noto CJK's 32px M is 24px high. Match it without moving the baseline.
    design = ImageFont.truetype(str(source), 33, layout_engine=ImageFont.Layout.BASIC)
    actual = ImageFont.truetype(str(source), round(33*pixels/32), layout_engine=ImageFont.Layout.BASIC)
    ascent, descent = design.getmetrics()
    alpha = Image.new('L', (8192, 8192), 0); draw = ImageDraw.Draw(alpha)
    records = []; x, y, row = 2, 2, 0
    for code in codes:
        left, top, right, bottom = actual.getbbox(chr(code), anchor='ls')
        w, h = right-left, bottom-top
        if x+w+2 > 8192: x, y, row = 2, y+row+4, 0
        if y+h+2 > 8192: raise ValueError('Latin atlas overflow')
        draw.text((x-left, y-top), chr(code), font=actual, anchor='ls', fill=255)
        dl, dt, dr, db = design.getbbox(chr(code), anchor='ls')
        records.append((code, 0, x, y, w, h, dl, ascent+dt, float(design.getlength(chr(code))), dr-dl, db-dt))
        x += w+4; row = max(row, h)
    path = output/f'graphics/fonts/reriff/latin/p{pixels}'
    base.write_texture(path, f'reriff_latin_{pixels}', alpha)
    if pixels == 32:
        data = struct.pack('<8I', 0x46435252, 1, len(records), 8192, 8192, 32, ascent, descent)
        data += b''.join(struct.pack('<I7f', r[0], *r[2:9]) for r in records)
    else:
        data = struct.pack('<12I', 0x46435252, 2, len(records), 8192, 8192, pixels, ascent, descent, 1, 44, 32, 0)
        data += b''.join(struct.pack('<2I9f', *r) for r in records)
    return data

def build():
    # Reuse the hash-verifying downloader with this pack's independent pin.
    import urllib.request, urllib.parse
    pin = json.loads((ROOT/'packaging/fonts/noto-latin.json').read_text())
    source = ROOT/'build/noto-latin'; source.mkdir(parents=True, exist_ok=True)
    for entry in pin['files']:
        path = source/Path(entry['path']).name
        if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest() != entry['sha256']:
            data = urllib.request.urlopen(pin['repository'].replace('github.com', 'raw.githubusercontent.com')+'/'+pin['commit']+'/'+urllib.parse.quote(entry['path']), timeout=60).read(entry['size']+1)
            if len(data) != entry['size'] or hashlib.sha256(data).hexdigest() != entry['sha256']: raise ValueError('Noto Sans download checksum mismatch')
            path.write_bytes(data)
    root = ROOT/'build/latin-component'; package = root/'package'
    if package.exists(): shutil.rmtree(package)
    fonts = package/'fonts/noto-latin'; fonts.mkdir(parents=True)
    resources = root/'textures'
    for pixels in (32, 64, 96):
        print('Baking Latin', pixels, flush=True)
        (fonts/f'latin{pixels}.rrcf').write_bytes(bake(resources, pixels))
    shutil.copy2(source/'OFL.txt', fonts/'OFL.txt')
    (package/'archives').mkdir()
    for path in archive.build_volume(resources, root/'volume', 'vfs1'):
        shutil.copy2(path, package/('archives/combined'+path.suffix))
    meta = dict(schema=2, component='latin', version=(ROOT/'VERSION').read_text().strip(), legacy={}, files={p.relative_to(package).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(package.rglob('*')) if p.is_file()})
    (package/'component.json').write_text(json.dumps(meta, indent=2)+'\n')
    with zipfile.ZipFile(root/'ReRiff-Latin.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for path in sorted(package.rglob('*')):
            if path.is_file(): z.write(path, path.relative_to(package).as_posix())
    print(root/'ReRiff-Latin.zip', flush=True)

if __name__ == '__main__': build()
