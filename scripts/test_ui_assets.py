#!/usr/bin/env python3
"""Check that application resource exports preserve the authored icon pixels."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ui_assets',ROOT/'scripts/build-ui-assets.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class Assets(unittest.TestCase):
    def test_exports(self):
        before={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'images').glob('*.png')}
        with tempfile.TemporaryDirectory() as temp:
            icon,bmp=module.build(Path(temp));data=icon.read_bytes();reserved,kind,count=struct.unpack_from('<3H',data)
            self.assertEqual((reserved,kind,count),(0,1,5))
            for i in range(count):
                w,h,_,_,planes,bits,length,offset=struct.unpack_from('<4B2H2I',data,6+i*16)
                self.assertEqual((planes,bits),(1,32));self.assertEqual(data[offset:offset+length],(ROOT/f'images/reriff-icon-{w}x{h}.png').read_bytes())
            with Image.open(Path(temp)/'reriff-logo-button.png') as outlined:
                self.assertEqual(outlined.getpixel((0,0))[3],0)
                self.assertEqual(outlined.getchannel('A').getextrema(),(0,255))
                self.assertTrue(any(pixel[:3]==(0,0,0) and pixel[3]>0 for pixel in (outlined.getpixel((x,y)) for y in range(outlined.height) for x in range(outlined.width))))
            with Image.open(bmp) as image:self.assertLessEqual(image.width,480);self.assertLessEqual(image.height,80)
        self.assertEqual(before,{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'images').glob('*.png')})
if __name__=='__main__':unittest.main()
