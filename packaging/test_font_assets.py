import hashlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

from font_assets import acquire, load_pin, validate_component


class FontAssetTests(unittest.TestCase):
    def setUp(self):
        stream = io.BytesIO()
        with zipfile.ZipFile(stream, 'w') as archive:
            archive.writestr('component.json', json.dumps({'schema': 2, 'version': '1.0.0'}))
        self.data = stream.getvalue()
        self.pin = {'schema': 1, 'repository': 'Aftonstjarna/ReRiff-Fonts', 'version': '1.0.0', 'packs': {}}
        for pack, asset in [('cjk', 'ReRiff-CJK.zip'), ('latin', 'ReRiff-Latin.zip')]:
            self.pin['packs'][pack] = {'asset': asset, 'size': len(self.data), 'sha256': hashlib.sha256(self.data).hexdigest()}
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)/'component.zip'

    def test_verified_cache_works_without_network_or_client_version(self):
        self.path.write_bytes(self.data)
        with patch('font_assets.urllib.request.urlopen', side_effect=AssertionError('Unexpected network')):
            self.assertEqual(acquire(self.path, self.pin, 'cjk'), self.path)

    def test_pinned_release_download(self):
        with patch('font_assets.urllib.request.urlopen', return_value=io.BytesIO(self.data)) as request:
            acquire(self.path, self.pin, 'cjk')
        self.assertEqual(request.call_args.args[0].full_url,
                         'https://github.com/Aftonstjarna/ReRiff-Fonts/releases/download/v1.0.0/ReRiff-CJK.zip')
        self.assertEqual(self.path.read_bytes(), self.data)

    def test_failed_download_preserves_previous_file(self):
        self.path.write_bytes(b'previous asset')
        with patch('font_assets.urllib.request.urlopen', return_value=io.BytesIO(b'wrong')):
            with self.assertRaises(ValueError):
                acquire(self.path, self.pin, 'cjk')
        self.assertEqual(self.path.read_bytes(), b'previous asset')
        self.assertFalse(list(self.path.parent.glob('*.download')))

    def test_oversize_download_refused(self):
        with patch('font_assets.urllib.request.urlopen', return_value=io.BytesIO(self.data+b'extra')):
            with self.assertRaises(ValueError):
                acquire(self.path, self.pin, 'cjk')
        self.assertFalse(self.path.exists())

    def test_metadata_mismatch_refused_even_with_matching_checksum(self):
        self.path.write_bytes(self.data)
        self.pin['version'] = '2.0.0'
        with self.assertRaises(ValueError):
            validate_component(self.path, self.pin, 'cjk')

    def test_invalid_repository_pin_refused(self):
        self.pin['repository'] = 'someone/else'
        pin = self.path.parent/'pin.json'
        pin.write_text(json.dumps(self.pin))
        with self.assertRaises(ValueError):
            load_pin(pin)
