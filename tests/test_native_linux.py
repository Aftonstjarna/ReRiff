"""Exercise the native reader with independent ZIP/config fixtures."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

BINARY = Path(sys.argv.pop(1)).resolve()


def config(records, pool, roots=1):
    return struct.pack('<4s4I', b'GFCF', 3, len(pool), len(records), roots) + b''.join(struct.pack('<8I', *r) for r in records) + pool


class NativeReaderTests(unittest.TestCase):
    def run_asset(self, data, mode='--config', compression=zipfile.ZIP_DEFLATED):
        with tempfile.TemporaryDirectory() as directory:
            with zipfile.ZipFile(Path(directory)/'vfs.zip', 'w', compression=compression) as archive:
                archive.writestr('scripts/test.cfg', data)
            args=[str(BINARY), directory, mode]
            if mode == '--config':
                args.append('scripts/test.cfg')
            return subprocess.run(args, capture_output=True, text=True)

    def test_nested_types_and_escaping(self):
        pool=b'\0root\0number\0text\0a"b\n\0'
        data=config([(1,0,1,0,0,2,2,0), (6,0,0x3f000000,0,0,1,0,0), (13,0,18,0,0,0,0,0)],pool)
        result=self.run_asset(data)
        self.assertEqual(result.returncode,0,result.stderr)
        nodes=json.loads(result.stdout)[0]['value']
        self.assertEqual(nodes[0]['value'],0.5)
        self.assertEqual(nodes[1]['value'],'a"b\n')

    def test_stored_zip(self):
        result=self.run_asset(config([(1,0,3,0,0,0,0,0)],b'\0x\0y\0'), compression=zipfile.ZIP_STORED)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(json.loads(result.stdout)[0]['value'],'y')

    def test_bad_configs(self):
        fixtures=[b'', b'GFCF', config([(1,0,0,0,0,2,1,0)],b'\0x\0'),
                  config([(999,0,0,0,0,0,0,0)],b'\0'),
                  config([(0,0,0,0,0,99,0,0)],b'\0'),
                  config([(0,0,0x7f800000,0,0,1,0,0)],b'\0'),
                  config([(0,0,0,0,0,0,0,0)],b'x')]
        for data in fixtures:
            with self.subTest(data=data):
                self.assertNotEqual(self.run_asset(data).returncode,0)

    def test_corrupt_archive(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'vfs.zip'
            with zipfile.ZipFile(path,'w') as archive:
                archive.writestr('asset',b'PAYLOAD')
            data=path.read_bytes().replace(b'PAYLOAD',b'CORRUPT')
            path.write_bytes(data)
            result=subprocess.run([str(BINARY),directory,'--verify'],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertIn('CRC mismatch',result.stderr)

    def test_truncated_archives(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'vfs.zip'
            for data in [b'',b'PK', b'\0'*40]:
                path.write_bytes(data)
                result=subprocess.run([str(BINARY),directory,'--verify'],capture_output=True,text=True)
                self.assertNotEqual(result.returncode,0)


if __name__ == '__main__':
    unittest.main()
