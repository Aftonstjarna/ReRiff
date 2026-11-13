"""Validate owned VFS indexes independently of the writer's tree traversal."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('native_archive', ROOT/'packaging/native_archive.py')
native = importlib.util.module_from_spec(spec)
spec.loader.exec_module(native)


class NativeArchiveTests(unittest.TestCase):
    def test_tree_metadata_and_determinism(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'input'
            assets = {'graphics/fonts/reriff/jp/batchtextures.brres': b'TABT'*50,
                      'graphics/fonts/reriff/kr/batchtextures.brres': b'Korean',
                      'scripts/fonts.bin': b'font metrics'}
            for name, data in assets.items():
                path = source/name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            index, archive = native.build_volume(source, root/'a', 'vfs1')
            raw = index.read_bytes()
            magic, nodes, count, strings, checksum = struct.unpack_from('<5I', raw)
            self.assertEqual(magic, 0x0300ffee)
            self.assertEqual(count, len(assets))
            self.assertEqual(len(raw), 28+nodes*8+count*28+strings)
            pool = raw[28+nodes*8+count*28:]
            seen, found = set(), {}

            def walk(node, prefix):
                while node != 0xffff:
                    self.assertLess(node, nodes)
                    self.assertNotIn(node, seen)
                    seen.add(node)
                    name, sibling, child, flags = struct.unpack_from('<4H', raw, 28+node*8)
                    component = pool[name:pool.index(b'\0', name)].decode('ascii')
                    path = prefix+component
                    if flags == 0x3fff:
                        walk(child, path+'/')
                    else:
                        self.assertLess(flags, count)
                        self.assertEqual(child, 0xffff)
                        found[flags] = path
                    node = sibling

            walk(struct.unpack_from('<H', raw, 24)[0], '')
            self.assertEqual(len(seen), nodes)
            self.assertEqual(set(found.values()), set(assets))
            central = 0
            with zipfile.ZipFile(archive) as bundle:
                self.assertIsNone(bundle.testzip())
                self.assertEqual(checksum, sum(i.CRC for i in bundle.infolist()) & 0xffffffff)
                for i, info in enumerate(bundle.infolist()):
                    self.assertEqual(bundle.read(info), assets[found[i]])
                    self.assertEqual(struct.unpack_from('<7I', raw, 28+nodes*8+i*28),
                                     (2, info.compress_type, info.compress_size, info.file_size,
                                      len(info.filename), info.header_offset, central))
                    central += 46+len(info.filename)+len(info.extra)+len(info.comment)
            second = native.build_volume(source, root/'b', 'vfs1')
            self.assertEqual(raw, second[0].read_bytes())
            self.assertEqual(archive.read_bytes(), second[1].read_bytes())

    def test_refuse_original_volume(self):
        for stem in ('vfs', 'vfs0', 'vfs20', 'other1'):
            with self.subTest(stem=stem), tempfile.TemporaryDirectory() as temp:
                with self.assertRaises(ValueError):
                    native.build_volume(Path(temp), Path(temp)/'output', stem)

    def test_refuse_unsupported_paths(self):
        for name in ('Uppercase.bin', '日本語.bin', 'back\\slash.bin'):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temp:
                source = Path(temp)/'input'
                source.mkdir()
                (source/name).write_bytes(b'bad')
                with self.assertRaises(ValueError):
                    native.build_volume(source, Path(temp)/'output', 'vfs1')


if __name__ == '__main__':
    unittest.main()
