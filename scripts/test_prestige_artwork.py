"""Verify the shipped artwork's encoded alpha, dimensions and native container."""
import hashlib
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('prestige_art', ROOT/'scripts/build-prestige-artwork.py')
art = importlib.util.module_from_spec(spec)
spec.loader.exec_module(art)

class PrestigeArtworkTests(unittest.TestCase):
    def test_authored_alpha_and_actual_native_dds(self):
        source = ROOT/'images/reriff-prestige-achievement.png'
        before = hashlib.sha256(source.read_bytes()).digest()
        with tempfile.TemporaryDirectory() as temp:
            target = Path(temp)/'texture.brres'
            art.build(source, target)
            raw = target.read_bytes()
            magic, endian, version, offset, count, pool, size = struct.unpack_from('<7I', raw)
            self.assertEqual((magic, endian, version, count), (0x54424154, 0xffee, 4, 1))
            self.assertEqual(offset, 52+pool)
            self.assertEqual(len(raw), offset+size)
            self.assertEqual(struct.unpack_from('<6I', raw, 28), (0, 0, size, 64, 64, 2))
            with Image.open(io.BytesIO(raw[offset:])) as texture:
                self.assertEqual(texture.size, (64, 64))
                self.assertEqual(texture.mode, 'RGBA')
                for point in ((0,0), (63,0), (0,63), (63,63), (2,2)):
                    self.assertEqual(texture.getpixel(point)[3], 0)
                self.assertEqual(texture.getpixel((32,32))[3], 255)
                # The edited source has authored transparent padding/rim.
                # BC2 retains alpha with four-bit quantization, not a new mask.
                with Image.open(source) as authored:
                    expected = authored.convert('RGBA').getchannel('A')
                    actual = texture.getchannel('A')
                    for x in range(64):
                        for y in range(64):
                            self.assertLessEqual(abs(actual.getpixel((x,y))-expected.getpixel((x,y))),16)
            second = Path(temp)/'repeat.brres'
            art.build(source, second)
            self.assertEqual(target.read_bytes(), second.read_bytes())
        self.assertEqual(hashlib.sha256(source.read_bytes()).digest(), before)

    def test_legacy_rgb_still_clips_opaque_corners(self):
        with tempfile.TemporaryDirectory() as temp:
            source, target = Path(temp)/'legacy.png', Path(temp)/'texture.brres'
            Image.new('RGB', (64,64), 'white').save(source)
            art.build(source,target)
            raw=target.read_bytes();offset=struct.unpack_from('<I',raw,12)[0]
            with Image.open(io.BytesIO(raw[offset:])) as texture:
                self.assertEqual(texture.getpixel((0,0))[3],0)
                self.assertEqual(texture.getpixel((32,32))[3],255)

if __name__ == '__main__':
    unittest.main()
