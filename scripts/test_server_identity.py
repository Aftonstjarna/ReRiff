"""Identity persistence and a one-time migration against isolated data stores."""
import concurrent.futures
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
import uuid
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SERVER = ROOT / 'vendor/riffapi-local-extended'
spec = importlib.util.spec_from_file_location('identity_fixture', SERVER / 'server_identity.py')
identity = importlib.util.module_from_spec(spec)
spec.loader.exec_module(identity)


class ServerIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.data = Path(self.temp.name)
        (self.data / 'classes/FileData').mkdir(parents=True)
        (self.data / 'files').mkdir()
        (self.data / 'files/track.bin').write_bytes(b'original track bytes')
        self.file = self.data / 'classes/FileData/existing-song.json'
        self.record = {'objectId': 'existing-song', 'username': 'existing-player',
                       'data': {'__type': 'File', 'name': 'track.bin',
                                'url': 'http://192.0.2.10:5144/files/track.bin'},
                       'unrelated': 'http://192.0.2.10:5144/admin'}
        self.original = json.dumps(self.record).encode()
        self.file.write_bytes(self.original)

    def test_identity_survives_restart_and_concurrent_initialization(self):
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            values = list(pool.map(lambda _: identity.server_id(self.data), range(32)))
        self.assertEqual(len(set(values)), 1)
        self.assertEqual(str(uuid.UUID(values[0])), values[0])
        self.assertEqual(identity.server_id(self.data), values[0])
        self.assertNotEqual(identity.server_id(self.data / 'another-store'), values[0])

    def test_migration_preserves_ids_files_and_original_backup(self):
        result = identity.migrate_file_origins(self.data, 'http://192.0.2.10:5144',
                                               'https://reriff.example.net/')
        changed = json.loads(self.file.read_text())
        self.assertEqual(changed['data']['url'], 'https://reriff.example.net/files/track.bin')
        self.assertEqual(changed['objectId'], self.record['objectId'])
        self.assertEqual(changed['username'], self.record['username'])
        self.assertEqual(changed['unrelated'], self.record['unrelated'])
        self.assertEqual((self.data / 'files/track.bin').read_bytes(), b'original track bytes')
        saved = next((self.data / 'migrations').glob('*/classes/FileData/existing-song.json'))
        self.assertEqual(saved.read_bytes(), self.original)
        again = identity.migrate_file_origins(self.data, result['from'], result['to'])
        self.assertEqual(result, again)
        self.assertEqual(result['changedObjects'], 1)
        self.assertEqual(saved.read_bytes(), self.original)

    def test_other_origins_and_missing_files_are_not_adopted(self):
        self.record['data']['url'] = 'https://another.example/files/track.bin'
        self.file.write_text(json.dumps(self.record))
        identity.migrate_file_origins(self.data, 'http://192.0.2.10:5144', 'https://reriff.example.net')
        self.assertEqual(json.loads(self.file.read_text()), self.record)

    def test_invalid_identity_is_not_replaced(self):
        (self.data / 'server-id').write_text('broken')
        with self.assertRaises(ValueError):
            identity.server_id(self.data)
        self.assertEqual((self.data / 'server-id').read_text(), 'broken')

    def test_invalid_record_aborts_before_changing_any_object(self):
        (self.file.parent / 'z-broken.json').write_text('broken')
        with self.assertRaises(ValueError):
            identity.migrate_file_origins(self.data, 'http://192.0.2.10:5144', 'https://reriff.example.net')
        self.assertEqual(self.file.read_bytes(), self.original)

    def test_hostnames_expose_same_identity_and_api_is_not_cacheable(self):
        module_spec = importlib.util.spec_from_file_location('identity_backend', SERVER / 'riffapi-local.py')
        backend = importlib.util.module_from_spec(module_spec)
        module_spec.loader.exec_module(backend)
        backend.DATA_DIR = str(self.data)
        backend.CLASSES_DIR = str(self.data / 'classes')
        backend.FILES_DIR = str(self.data / 'files')
        backend.LOG_FILE = str(self.data / 'requests.log')
        with patch.dict(os.environ, {'RERIFF_MIGRATE_FROM_ORIGIN': ''}):
            backend.ensure_dirs()
        client = backend.app.test_client()
        values = []
        for host in ('192.168.0.211:5144', 'reriff.example.net'):
            response = client.get('/classes/Version', headers={'Host': host})
            values.append(response.json['results'][0]['ServerID'])
            self.assertEqual(response.json['results'][0]['Ver'], 100)
            self.assertEqual(response.headers['X-ReRiff-Server-ID'], values[-1])
            self.assertIn('no-store', response.headers['Cache-Control'])
        self.assertEqual(values[0], values[1])
        self.assertEqual(client.get('/').json['serverId'], values[0])

    def test_public_and_lan_files_share_store_without_rewriting_it(self):
        module_spec = importlib.util.spec_from_file_location('alias_backend', SERVER / 'riffapi-local.py')
        backend = importlib.util.module_from_spec(module_spec)
        module_spec.loader.exec_module(backend)
        backend.DATA_DIR = str(self.data)
        backend.CLASSES_DIR = str(self.data / 'classes')
        backend.FILES_DIR = str(self.data / 'files')
        backend.LOG_FILE = str(self.data / 'requests.log')
        backend.FILE_ORIGINS = {'public': 'https://reriff.example.net', 'lan': 'http://192.0.2.10:5144'}
        client = backend.app.test_client()
        before = self.file.read_bytes()
        ids = []
        # No external network is needed for either view of these existing records.
        with patch('urllib.request.urlopen', side_effect=AssertionError('Unexpected network access')):
            for host, expected in [('reriff.example.net', 'https://reriff.example.net'),
                                   ('192.0.2.10:5144', 'http://192.0.2.10:5144')]:
                response = client.get('/classes/FileData/existing-song', headers={'Host': host})
                self.assertEqual(response.status_code, 200)
                self.assertEqual(response.json['data']['url'], expected + '/files/track.bin')
                ids.append(response.headers['X-ReRiff-Server-ID'])
                uploaded = client.post('/files/new.bin', data=b'shared bytes', headers={'Host': host})
                self.assertEqual(uploaded.json['url'], expected + '/files/new.bin')
                downloaded = client.get('/files/new.bin', headers={'Host': host})
                self.assertEqual(downloaded.data, b'shared bytes')
        self.assertEqual(ids[0], ids[1])
        self.assertEqual(self.file.read_bytes(), before)
        self.assertEqual(client.get('/', headers={'Host': 'reriff.example.net'}).json['addresses'], backend.FILE_ORIGINS)

    def test_forwarded_host_cannot_invent_an_unconfigured_origin(self):
        aliases = {'public': 'https://reriff.example.net', 'lan': 'http://192.0.2.10:5144'}
        self.assertEqual(identity.client_origin(aliases, 'reriff.example.net', 'http://reriff.example.net/'), aliases['public'])
        self.assertEqual(identity.client_origin(aliases, '192.0.2.10:5144', 'http://192.0.2.10:5144/'), aliases['lan'])
        self.assertEqual(identity.client_origin(aliases, 'unknown.example', 'http://unknown.example/'), aliases['public'])
        value = {'__type': 'File', 'name': 'track.bin', 'url': 'https://unrelated.example/files/track.bin'}
        self.assertFalse(identity.remap_file_urls(value, set(aliases.values()), aliases['lan'], self.data / 'files'))


if __name__ == '__main__':
    unittest.main()
