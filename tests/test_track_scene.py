"""Independent geometry and placement checks for the recovered straight-road path."""
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'NativeLinux'))
from track_scene import road_profile,wall_profile,strip_indices,normal,palette_from_config,write_mesh,config
from assets import FormatError


def palette_fixture():
    pool=bytearray(b'\0');names={'':0}
    for key in ('dance','demo','road','base','accent','pulse'):
        names[key]=len(pool);pool.extend(key.encode()+b'\0')
    tables=[('dance',1,1),('demo',2,1),('road',3,3),('base',6,3),('accent',9,3),('pulse',12,3)]
    records=[(names[name],0,first,0,0,2,count,0) for name,first,count in tables]
    for v in (20,40,60,80,100,120,140,160,180):
        records.append((0,0,struct.unpack('<I',struct.pack('<f',v))[0],0,0,1,0,0))
    return struct.pack('<4s4I',b'GFCF',3,len(pool),len(records),1)+b''.join(struct.pack('<8I',*r) for r in records)+pool


class TrackSceneTests(unittest.TestCase):
    def test_original_cross_section_and_width_remap(self):
        self.assertEqual(road_profile(),list(range(-12,13,3)))
        narrowed=road_profile(0,1)
        self.assertEqual(narrowed[0],-12)
        self.assertAlmostEqual(narrowed[-1],4,places=5)
        self.assertEqual(narrowed[4],0)
        with self.assertRaises(FormatError):road_profile(-1,1)

    def test_original_triangle_order_faces_up(self):
        triangles=strip_indices(2,9)
        self.assertEqual(len(triangles)//3,16)
        self.assertEqual(triangles[:6],[0,9,1,9,10,1])
        points=[(x,0,z) for z in (0,3) for x in road_profile()]
        for j in range(0,len(triangles),3):
            self.assertEqual(normal(*(points[i] for i in triangles[j:j+3])),(0,1,0))

    def test_wall_cross_section_matches_recovered_steps(self):
        left,right=wall_profile(1),wall_profile(-1)
        self.assertEqual(len(left),7)
        self.assertEqual(left[0],(12,-1))
        self.assertEqual(left[1],(12,0.75))
        self.assertEqual(left[-1],(0,-8))
        for a,b in zip(left,right):self.assertEqual(a,(-b[0],b[1]))
        self.assertAlmostEqual(left[4][0],12.625)
        self.assertAlmostEqual(left[4][1],0.5)

    def test_recovered_palette_and_malformed_config(self):
        data=palette_fixture()
        name,palette=palette_from_config(data,'dance')
        self.assertEqual(name,'demo')
        self.assertEqual(palette['base'],[20/255,40/255,60/255])
        self.assertEqual(palette['pulse'],[140/255,160/255,180/255])
        bad=bytearray(data);struct.pack_into('<I',bad,28,0)
        with self.assertRaises(FormatError):config(bad)
        with self.assertRaises(FormatError):config(data[:-1])

    def test_runtime2_preserves_texture_coordinate_and_bounds(self):
        vertices=[(x,0,z,0,1,0,1,1,1,0,0.5,u,v,1) for x,z,u,v in ((-12,0,0,0),(12,0,1,0),(-12,3,0,0.24))]
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'sample.rmesh'
            write_mesh(path,vertices,[0,1,2])
            data=path.read_bytes()
        magic,count,indices,*bounds=struct.unpack_from('<8sII6f',data)
        self.assertEqual((magic,count,indices),(b'RRMESH2\0',3,3))
        self.assertEqual(bounds,[-12,0,0,12,0,3])
        self.assertEqual(len(data),40+3*56+3*4)
        self.assertEqual(struct.unpack_from('<3f',data,40+44),(0,0,1))


if __name__=='__main__':unittest.main()
