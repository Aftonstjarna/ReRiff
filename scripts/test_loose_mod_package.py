#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location('package', Path(__file__).with_name('package-loose-mod.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class PackageTests(unittest.TestCase):
    def test_distribution_and_guards(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            assets = root / 'assets'
            assets.mkdir()
            (assets / 'asset.txt').write_text('native asset')
            output = root / 'mod.zip'
            module.package(assets, output, 'example')
            with zipfile.ZipFile(output) as archive:
                self.assertEqual(archive.namelist(), ['example/assets/asset.txt'])
                self.assertEqual(archive.read(archive.namelist()[0]), b'native asset')
            before = output.read_bytes()
            with self.assertRaises(FileExistsError):
                module.package(assets, output, 'example')
            self.assertEqual(output.read_bytes(), before)
            with self.assertRaises(ValueError):
                module.package(assets, assets / 'bad.zip', 'example')
            with self.assertRaises(ValueError):
                module.package(assets, root / 'bad.zip', '../escape')
            (assets / 'vfs.zip').write_bytes(b'archive')
            with self.assertRaises(ValueError):
                module.package(assets, root / 'bad.zip', 'example')
            (assets / 'vfs.zip').unlink()
            (assets / 'link').symlink_to(output)
            with self.assertRaises(ValueError):
                module.package(assets, root / 'bad.zip', 'example')
            self.assertFalse((root / 'bad.zip').exists())

if __name__ == '__main__':
    unittest.main()
