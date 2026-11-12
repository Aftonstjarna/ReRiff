"""Check encoded atlas pixels and metrics against the pinned source face."""
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest

from PIL import Image, ImageFont

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('cjk_atlas', ROOT/'scripts/build-cjk-atlas.py')
atlas = importlib.util.module_from_spec(spec)
spec.loader.exec_module(atlas)
SOURCE = ROOT/'build/noto-cjk/NotoSansCJK-Regular.ttc'


class CjkAtlasTests(unittest.TestCase):
    def test_regional_metrics_pixels_and_supplementary_codepoint(self):
        text = ' ?く月ド한漢體\U0002000b'
        for region in atlas.REGIONS:
            with self.subTest(region=region), tempfile.TemporaryDirectory() as temp:
                output = Path(temp)
                summary = atlas.bake(SOURCE, output, region, characters=text)
                self.assertEqual(summary['supplementary'], 1)
                path = output/'graphics/fonts/reriff/cjk'/region
                raw = (path/'batchtextures.brres').read_bytes()
                magic, marker, version, offset, count, names, size = struct.unpack_from('<7I', raw)
                self.assertEqual((magic, marker, version, count), (0x54424154, 0xffee, 4, 1))
                self.assertEqual(offset, 52+names)
                self.assertEqual(len(raw), offset+size)
                self.assertEqual(offset % 32, 0)
                texture = Image.open(io.BytesIO(raw[offset:])).convert('RGBA')
                metrics = (path/'glyphs.rrcf').read_bytes()
                header = struct.unpack_from('<8I', metrics)
                self.assertEqual(header[:3], (0x46435252, 1, len(set(text))))
                self.assertEqual(header[3:5], texture.size)
                self.assertEqual(len(metrics), 32+header[2]*32)
                font = ImageFont.truetype(str(SOURCE), 32, index=atlas.REGIONS.index(region),
                                         layout_engine=ImageFont.Layout.BASIC)
                codes = []
                for i in range(header[2]):
                    code, x, y, w, h, left, top, advance = struct.unpack_from('<I7f', metrics, 32+i*32)
                    codes.append(code)
                    self.assertEqual(advance, font.getlength(chr(code)))
                    self.assertGreaterEqual(x, 2)
                    self.assertGreaterEqual(y, 2)
                    self.assertLessEqual(x+w, texture.width-2)
                    self.assertLessEqual(y+h, texture.height-2)
                    box = (int(x), int(y), int(x+w), int(y+h))
                    if chr(code) != ' ':
                        self.assertIsNotNone(texture.getchannel('A').crop(box).getbbox())
                self.assertEqual(codes, sorted(set(map(ord, text))))


if __name__ == '__main__':
    unittest.main()
