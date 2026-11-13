"""Check the independent Polish pack and canonical HD layout metrics."""
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('latin',ROOT/'scripts/build-latin-component.py')
latin=importlib.util.module_from_spec(spec);spec.loader.exec_module(latin)
class LatinTests(unittest.TestCase):
    def test_polish_pixels_and_hd_metrics(self):
        chars='ĄąĆćĘęŁłŃńÓóŚśŹźŻż'
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);before={}
            for pixels in (32,64,96):
                raw=latin.bake(root,pixels,chars)
                texture=(root/f'graphics/fonts/reriff/latin/p{pixels}/batchtextures.brres').read_bytes()
                offset=struct.unpack_from('<I',texture,12)[0]
                alpha=Image.open(io.BytesIO(texture[offset:])).convert('RGBA').getchannel('A')
                count=struct.unpack_from('<I',raw,8)[0];self.assertEqual(count,len(chars))
                for i in range(count):
                    if pixels==32:
                        code,x,y,w,h,left,top,advance=struct.unpack_from('<I7f',raw,32+i*32)
                        before[code]=(left,top,advance,w,h)
                    else:
                        code,page,x,y,w,h,left,top,advance,dw,dh=struct.unpack_from('<2I9f',raw,48+i*44)
                        self.assertEqual(page,0);self.assertEqual((left,top,advance,dw,dh),before[code])
                    self.assertIsNotNone(alpha.crop((int(x),int(y),int(x+w),int(y+h))).getbbox())
                self.assertIn(ord('ł'),before)
if __name__=='__main__':unittest.main()
