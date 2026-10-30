"""Synthetic fixtures test the decoded layouts without redistributing assets."""
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'NativeLinux'))
from assets import decode_eadb, decode_tabt, export_gltf, FormatError


def eadb():
    data=bytearray(878)
    struct.pack_into('<4s6I',data,0,b'EADB',38,1,1,0,1,1)
    struct.pack_into('<9I',data,64,3,128,592,592,748,748,796,800,872)
    data[128:132]=b'mat\0'
    data[592:597]=b'body\0'
    struct.pack_into('<12f',data,624,1,0,0,0,1,0,0,0,1,10,20,30)
    struct.pack_into('<6I',data,748,0x30003000,24,0,3,0,3)
    struct.pack_into('<2h',data,796,0,-1)
    for i,point in enumerate(((0,0,0),(1,0,0),(0,0,1))):
        struct.pack_into('<3f6e',data,800+i*24,*point,0,1,0,1,0.25,0.75)
    struct.pack_into('<3H',data,872,0,1,2)
    return data


def tabt():
    dds=bytearray(136)
    dds[:4]=b'DDS '
    struct.pack_into('<I',dds,4,124)
    struct.pack_into('<2I',dds,12,4,4)
    struct.pack_into('<I',dds,28,1)
    dds[84:88]=b'DXT1'
    return bytearray(struct.pack('<4s6I',b'TABT',0xffee,4,54,1,2,len(dds))+struct.pack('<6I',0,0,len(dds),4,4,0)+b'x\0'+dds)


class VehicleAssets(unittest.TestCase):
    def test_mesh_positions_uv_indices_and_nodes(self):
        model=decode_eadb(eadb())
        mesh=model['meshes'][0]
        self.assertEqual(mesh['positions'][1],(1,0,0))
        self.assertEqual(mesh['uvs'][0],(0.25,0.75))
        self.assertEqual(mesh['indices'],[0,1,2])
        self.assertEqual(model['nodes'][0]['transform'][-3:],(10,20,30))

    def test_gltf_buffer_and_transform(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)
            export_gltf(decode_eadb(eadb()),[],path)
            gltf=json.loads((path/'vehicle.gltf').read_text())
            binary=(path/'vehicle.bin').read_bytes()
            self.assertEqual(len(binary),gltf['buffers'][0]['byteLength'])
            self.assertEqual(gltf['nodes'][0]['matrix'][12:15],[10,20,30])
            for view in gltf['bufferViews']:
                self.assertEqual(view['byteOffset']%4,0)
                self.assertLessEqual(view['byteOffset']+view['byteLength'],len(binary))
            self.assertEqual(gltf['accessors'][0]['max'],[1,0,1])

    def test_inspection_excludes_effect_nodes(self):
        model=decode_eadb(eadb())
        model['nodes'].append(dict(name='electric',transform=model['nodes'][0]['transform'],meshes=[0]))
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)
            export_gltf(model,[],path)
            full=json.loads((path/'vehicle.gltf').read_text())
            clean=json.loads((path/'vehicle-inspection.gltf').read_text())
            self.assertIn('electric',[n['name'] for n in full['nodes']])
            self.assertNotIn('electric',[n['name'] for n in clean['nodes']])
            self.assertEqual(clean['scenes'][0]['nodes'],[0])
            self.assertEqual(clean['nodes'][0]['children'],[1])
            self.assertEqual(clean['nodes'][1]['mesh'],0)
            runtime=(path/'vehicle.rmesh').read_bytes()
            magic,vertices,indices,*bounds=struct.unpack_from('<8sII6f',runtime)
            self.assertEqual((magic,vertices,indices),(b'RRMESH1\0',3,3))
            self.assertEqual(bounds,[10,20,30,11,20,31])
            self.assertEqual(struct.unpack_from('<3f',runtime,40),(10,20,30))
            self.assertEqual(len(runtime),40+vertices*44+indices*4)

    def test_mesh_rejects_bad_layouts(self):
        for offset,value in [(4,37),(92,99999),(752,20),(768,4),(712,99999)]:
            data=eadb()
            struct.pack_into('<I',data,offset,value)
            with self.subTest(offset=offset),self.assertRaises(FormatError):
                decode_eadb(data)

    def test_mesh_rejects_index_out_of_range(self):
        data=eadb()
        struct.pack_into('<H',data,872,3)
        with self.assertRaises(FormatError):
            decode_eadb(data)

    def test_texture_boundaries(self):
        result=decode_tabt(tabt())[0]
        self.assertEqual((result['name'],result['width'],result['fourcc']),('x',4,'DXT1'))
        for offset,value in [(8,3),(12,99999),(32,99999),(40,8)]:
            data=tabt()
            struct.pack_into('<I',data,offset,value)
            with self.subTest(offset=offset),self.assertRaises(FormatError):
                decode_tabt(data)

    def test_truncated_files(self):
        for function,fixture in [(decode_eadb,eadb()),(decode_tabt,tabt())]:
            for length in [0,4,27,len(fixture)-1]:
                with self.subTest(length=length,function=function),self.assertRaises(FormatError):
                    function(fixture[:length])

    @unittest.skipUnless(os.environ.get('RERIFF_GAME_DIR'),'set RERIFF_GAME_DIR for installed-asset validation')
    def test_installed_vehicle_layouts(self):
        with zipfile.ZipFile(Path(os.environ['RERIFF_GAME_DIR'])/'vfs.zip') as archive:
            paths=[n for n in archive.namelist() if n.startswith('graphics/vehicles/') and n.endswith('/lod0_livery.bdae')]
            self.assertGreater(len(paths),0)
            for path in paths:
                with self.subTest(path=path):
                    model=decode_eadb(archive.read(path))
                    self.assertGreater(len(model['meshes']),0)
                    textures=decode_tabt(archive.read(path.rsplit('/',1)[0]+'/textures/batchTextures.brres'))
                    self.assertGreater(len(textures),0)


if __name__=='__main__':
    unittest.main()
