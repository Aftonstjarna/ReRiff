"""Regression tests for cross-client cached song IDs using isolated storage."""
import importlib.util
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SongAliasesTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.data = Path(self.temp.name)
        spec = importlib.util.spec_from_file_location('alias_test_server', ROOT/'vendor/riffapi-local-extended/riffapi-local.py')
        self.server = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.server)
        self.server.DATA_DIR = str(self.data)
        self.server.CLASSES_DIR = str(self.data/'classes')
        self.server.FILES_DIR = str(self.data/'files')
        self.server.LOG_FILE = str(self.data/'requests.log')
        self.server.ensure_dirs()
        self.client = self.server.app.test_client()
        self.ids = ['cached-first', 'cached-second']
        self.scores = []
        for song, owner, score in [('cached-first', 'first', 195172), ('cached-second', 'second', 28200), ('unrelated', 'third', 999999)]:
            obj = self.client.post('/classes/Score', json={'SongID': song, 'Owner': owner, 'Score': score, 'Mode': 0}).get_json()
            self.scores.append(obj['objectId'])
            self.client.post('/classes/ghost_v18', json={'song': song, 'user': owner, 'score': score})

    def query(self, cls, where, **params):
        return self.client.get('/classes/'+cls, query_string={'where': json.dumps(where), **params}).get_json()

    def enable(self):
        (self.data/'song-aliases.json').write_text(json.dumps([self.ids]))

    def test_both_clients_get_shared_sorted_scores_and_ghosts(self):
        self.enable()
        for song in self.ids:
            for cls, field, order in [('Score', 'SongID', '-Score'), ('ghost_v18', 'song', '-score')]:
                rows = self.query(cls, {field: song}, order=order)['results']
                self.assertEqual(len(rows), 2)
                self.assertEqual([r.get('Score', r.get('score')) for r in rows], [195172, 28200])
            self.assertEqual(len(self.query('Score', {'SongID': song, 'Owner': {'$in': ['first', 'second']}})['results']), 2)
            self.assertEqual(self.query('Score', {'SongID': song, 'Score': {'$gt': 28200}}, count=1, limit=0), {'count': 1, 'results': []})

    def test_mapping_reload_owner_update_and_original_records(self):
        self.assertEqual(len(self.query('Score', {'SongID': self.ids[0]})['results']), 1)
        self.enable()
        rows = self.query('Score', {'SongID': self.ids[1], 'Owner': 'first'})['results']
        self.assertEqual(rows[0]['objectId'], self.scores[0])
        self.assertEqual(rows[0]['SongID'], self.ids[0])
        self.assertEqual(self.client.put('/classes/Score/'+self.scores[0], json={'Score': 200000}).status_code, 200)
        self.assertEqual(self.query('Score', {'SongID': self.ids[1], 'Owner': 'first'})['results'][0]['Score'], 200000)
        (self.data/'song-aliases.json').unlink()
        self.assertEqual(self.query('Score', {'SongID': self.ids[1], 'Owner': 'first'})['results'], [])

    def test_operator_equivalence_and_unrelated_fields(self):
        self.enable()
        self.assertEqual(len(self.query('Score', {'SongID': {'$in': [self.ids[1]]}})['results']), 2)
        self.assertEqual(len(self.query('Score', {'SongID': {'$ne': self.ids[1]}})['results']), 1)
        self.assertEqual(len(self.query('Score', {'SongID': 'unrelated'})['results']), 1)
        self.client.post('/classes/Other', json={'SongID': self.ids[0]})
        self.assertEqual(self.query('Other', {'SongID': self.ids[1]})['results'], [])

    def test_rejects_ambiguous_mapping(self):
        (self.data/'song-aliases.json').write_text(json.dumps([self.ids, [self.ids[1], 'third']]))
        with self.assertRaises(ValueError):
            self.server.song_aliases()

    def test_exact_source_registration_joins_history_without_manual_mapping(self):
        digest = hashlib.sha256(b'identical-source-audio').hexdigest()
        songs = [{'song_id': song, 'sha256': digest, 'title': 'Same song', 'artist': 'Same artist'} for song in self.ids]
        for song in songs:
            response = self.client.post('/reriff/songs/register', json={'songs': [song]})
            self.assertEqual(response.status_code, 200, response.get_json())
        self.assertFalse((self.data/'song-aliases.json').exists())
        for song in self.ids:
            self.assertEqual(len(self.query('Score', {'SongID': song})['results']), 2)
            self.assertEqual(len(self.query('ghost_v18', {'song': song})['results']), 2)
        # Catalog persistence, not process state, drives identity.
        state = json.loads((self.data/'song-catalog.json').read_text())
        self.assertEqual(len(state['records']), 2)
        self.assertEqual(len(set(state['aliases'].values())), 1)
        payload = b'original ghost bytes\x00\xff'
        upload = self.client.post('/files/cached-first-123.ghost', data=payload)
        self.assertEqual(upload.status_code, 201)
        url = upload.get_json()['url']
        self.client.post('/classes/FileData', json={'name': 'cached-first-123.ghost', 'data': {'__type': 'File', 'url': url}})
        files = self.query('FileData', {'name': 'cached-second-123.ghost'})['results']
        self.assertEqual(files[0]['data']['url'], url)
        self.assertEqual(self.client.get('/files/cached-second-123.ghost').data, payload)
        self.assertEqual(self.client.get('/files/unrelated-123.ghost').status_code, 404)

    def test_matching_metadata_does_not_join_different_audio(self):
        songs = [{'song_id': song, 'sha256': hashlib.sha256(song.encode()).hexdigest(), 'title': 'Same', 'artist': 'Same'} for song in self.ids]
        self.assertEqual(self.client.post('/reriff/songs/register', json={'songs': songs}).status_code, 200)
        self.assertEqual(len(self.query('Score', {'SongID': self.ids[0]})['results']), 1)
        original = (self.data/'song-catalog.json').read_bytes()
        songs[0]['sha256'] = '0'*64
        response = self.client.post('/reriff/songs/register', json={'songs': songs})
        self.assertEqual(response.status_code, 409)
        self.assertEqual((self.data/'song-catalog.json').read_bytes(), original)

    def test_invalid_manifests_do_not_change_history(self):
        for body in [{}, {'songs': 'bad'}, {'songs': [{'song_id': '../bad', 'sha256': '0'*64}]}]:
            self.assertIn(self.client.post('/reriff/songs/register', json=body).status_code, (400, 409))
        self.assertFalse((self.data/'song-catalog.json').exists())
        self.assertEqual(len(self.query('Score', {'SongID': self.ids[0]})['results']), 1)

    def test_duplicate_player_counts_best_score_once(self):
        self.enable()
        self.client.post('/classes/Score', json={'SongID': self.ids[1], 'Owner': 'first', 'Score': 190000, 'Mode': 0})
        rows = self.query('Score', {'SongID': self.ids[1]}, order='-Score', count=1)
        self.assertEqual(rows['count'], 2)
        self.assertEqual([o['Score'] for o in rows['results']], [195172, 28200])
        self.assertEqual(len(self.server.load_all_objects('Score')), 4)


if __name__ == '__main__':
    unittest.main()
