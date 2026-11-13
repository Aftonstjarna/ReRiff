"""Probe the maintained backend with isolated Unicode metadata storage."""
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


class MetadataUnicodeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.data = Path(self.temp.name)
        spec = importlib.util.spec_from_file_location('metadata_probe_server', ROOT/'vendor/riffapi-local-extended/riffapi-local.py')
        self.server = importlib.util.module_from_spec(spec)
        with patch.dict(os.environ, {'RERIFF_DATA_DIR': str(self.data),
                                    'RERIFF_CATALOG_LIBRARY': str(ROOT/'build/server-sync/libreriff-catalog.so')}):
            spec.loader.exec_module(self.server)
        self.server.ensure_dirs()
        self.client = self.server.app.test_client()

    def test_parse_create_read_update_query_and_disk_reload(self):
        for escaped in (False, True):
            with self.subTest(escaped=escaped):
                original = dict(SongID='unicode-probe-'+str(escaped),
                                SongName='くちづけ 月下麗人 ドレス 漢字 한글 \U0002000b 😀 e\u0301',
                                ArtistName='Artist‐Name', AlbumName='繁體 简体 日本語')
                response = self.client.post('/classes/Score',
                    data=json.dumps(original, ensure_ascii=escaped).encode('utf-8'), content_type='application/json')
                self.assertEqual(response.status_code, 200)
                identity = response.get_json()['objectId']
                stored = self.client.get('/classes/Score/'+identity).get_json()
                for key, value in original.items():
                    self.assertEqual(stored[key], value)
                query = self.client.get('/classes/Score', query_string={
                    'where': json.dumps({'SongName': original['SongName']}, ensure_ascii=escaped)}).get_json()
                self.assertTrue(any(row['objectId'] == identity for row in query['results']))
                changed = '更新 한국어 \U0002000b'
                self.assertEqual(self.client.put('/classes/Score/'+identity,
                    data=json.dumps({'SongName': changed}, ensure_ascii=escaped).encode('utf-8'),
                    content_type='application/json').status_code, 200)
                rows = self.server.load_all_objects('Score')
                saved = next(row for row in rows if row['objectId'] == identity)
                self.assertEqual(saved['SongName'], changed)
                self.assertEqual(saved['ArtistName'], original['ArtistName'])
                # Decode persisted bytes independently of the server's loader.
                disk = json.loads((self.data/'classes/Score'/(identity+'.json')).read_bytes().decode('utf-8'))
                self.assertEqual(disk['SongName'], changed)

    def test_native_catalog_preserves_title_and_artist(self):
        song = dict(song_id='unicode-probe', sha256='a'*64,
                    title='くちづけ 漢字 한글 \U0002000b 😀', artist='Artist‐Name')
        result = self.client.post('/reriff/songs/register', json={'songs': [song]})
        self.assertEqual(result.status_code, 200)
        disk = json.loads((self.data/'song-catalog.json').read_bytes().decode('utf-8'))
        self.assertEqual(disk['records'][song['song_id']]['title'], song['title'])
        self.assertEqual(disk['records'][song['song_id']]['artist'], song['artist'])

    def test_live_track_field_names_preserve_unicode(self):
        metadata = {'Title': 'ひきずるビート 漢字 한글', 'Artist': 'Artist‐Name', 'Genre': 'rock'}
        created = self.client.post('/classes/Track', json=metadata).get_json()
        identity = created['objectId']
        returned = self.client.get('/classes/Track/'+identity).get_json()
        self.assertEqual({key: returned[key] for key in metadata}, metadata)
        rows = self.client.get('/classes/Track', query_string={
            'where': json.dumps({'Title': metadata['Title'], 'Artist': metadata['Artist']}, ensure_ascii=False)}).get_json()['results']
        self.assertEqual([row['objectId'] for row in rows], [identity])


if __name__ == '__main__':
    unittest.main()
