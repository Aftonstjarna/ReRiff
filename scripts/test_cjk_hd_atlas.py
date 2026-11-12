"""Validate high-detail page boundaries and exact canonical layout metrics."""
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('hd_atlas', ROOT/'scripts/build-cjk-hd-atlas.py')
hd = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hd)
SOURCE = ROOT/'build/noto-cjk/NotoSansCJK-Regular.ttc'


class HdAtlasTests(unittest.TestCase):
    def test_paged_pixels_and_unchanged_layout(self):
        chars = ' ?ミスターフィクサー漢字月下麗人くちづけ한글體\U0002000b'
        for pixels in (64, 96):
            with self.subTest(pixels=pixels), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                old = hd.base.bake(SOURCE, root/'base', 'jp', characters=chars)
                result = hd.bake(SOURCE, root/'hd', pixels=pixels, characters=chars, page_size=256)
                self.assertGreater(result['pages'], 1)
                before = (root/'base/graphics/fonts/reriff/cjk/jp/glyphs.rrcf').read_bytes()
                target = root/f'hd/graphics/fonts/reriff/cjk/jp{pixels}'
                raw = (target/'glyphs.rrcf').read_bytes()
                header = struct.unpack_from('<12I', raw)
                self.assertEqual(header[:3], (0x46435252, 2, old['codepoints']))
                self.assertEqual(header[8:11], (result['pages'], 44, 32))
                textures = []
                for page in range(result['pages']):
                    texture = (target/f'p{page:03}/batchtextures.brres').read_bytes()
                    offset = struct.unpack_from('<I', texture, 12)[0]
                    textures.append(Image.open(io.BytesIO(texture[offset:])).convert('RGBA'))
                seen = set()
                for i in range(header[2]):
                    code, page, x, y, w, h, left, top, advance, dw, dh = struct.unpack_from('<2I9f', raw, 48+i*44)
                    original = struct.unpack_from('<I7f', before, 32+i*32)
                    self.assertEqual((code, left, top, advance, dw, dh),
                                     (original[0], original[5], original[6], original[7], original[3], original[4]))
                    self.assertLess(page, result['pages'])
                    self.assertLessEqual(x+w+2, 256)
                    self.assertLessEqual(y+h+2, 256)
                    if h and w:
                        self.assertIsNotNone(textures[page].getchannel('A').crop((int(x), int(y), int(x+w), int(y+h))).getbbox())
                    seen.add(code)
                self.assertIn(0x2000b, seen)
                self.assertEqual(len(seen), header[2])


if __name__ == '__main__':
    unittest.main()
