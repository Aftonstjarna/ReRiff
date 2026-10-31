"""Exercise the native News policy through the real Flask query route."""
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


class NewsVersions(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.native = patch.dict(os.environ, {'RERIFF_NEWS_LIBRARY': str(ROOT/'build/server-sync/libreriff-news.so'),
                                             'RERIFF_SERVER_VERSION': '0.1.9'})
        self.native.start()
        self.addCleanup(self.native.stop)
        spec = importlib.util.spec_from_file_location('news_server', ROOT/'vendor/riffapi-local-extended/riffapi-local.py')
        self.server = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.server)
        for key, suffix in [('DATA_DIR', ''), ('CLASSES_DIR', 'classes'), ('FILES_DIR', 'files')]:
            setattr(self.server, key, str(Path(self.temp.name)/suffix))
        self.server.LOG_FILE = str(Path(self.temp.name)/'requests.log')
        self.server.ensure_dirs()
        self.server.store_object('News', {'objectId': 'announcement', 'ID': 17, 'Status': 'Active',
                                         'enTitle': 'Normal news', 'en': 'Normal body', 'fr': 'Texte'})
        self.server.store_object('News', {'objectId': 'RiffApiLocalNews', 'ID': 16, 'Status': 'Active'})
        self.client = self.server.app.test_client()

    def news(self, version=None, query=None):
        response = self.client.get('/classes/News', headers={'X-ReRiff-Client-Version': version} if version else {},
                                   query_string=query or {})
        self.assertEqual(response.status_code, 200)
        rows = response.json['results']
        self.assertNotIn('RiffApiLocalNews', [row['objectId'] for row in rows])
        return rows

    def test_newer_server_recommends_exit_and_preserves_news(self):
        rows = self.news('0.1.8')
        self.assertEqual(rows[0]['enTitle'], 'Exit to update ReRiff')
        self.assertIn('Exit the game', rows[0]['en'])
        self.assertIn('0.1.8', rows[0]['en'])
        self.assertEqual(rows[1]['fr'], 'Texte')
        for language in ('en', 'de', 'ru', 'es', 'it', 'fr'):
            self.assertEqual(rows[0][language], rows[0]['en'])

    def test_equal_or_newer_clients_do_not_get_update_prompt(self):
        for version in ('0.1.9', '0.1.10', '1.0.0'):
            self.assertNotIn('Exit the game', self.news(version)[0]['en'])

    def test_numeric_comparison(self):
        with patch.dict(os.environ, {'RERIFF_SERVER_VERSION': '0.10.0'}):
            self.assertIn('Exit the game', self.news('0.9.9')[0]['en'])

    def test_unknown_or_invalid_versions_do_not_invent_update(self):
        for version in (None, 'not-a-version', '1.2.3\"bad', '10000.0.0', '1.2'):
            self.assertIn('unknown', self.news(version)[0]['en'])
        with patch.dict(os.environ, {'RERIFF_SERVER_VERSION': 'invalid'}):
            self.assertIn('unknown', self.news('0.1.8')[0]['en'])

    def test_normal_parse_filters_and_count(self):
        rows = self.news('0.1.8', {'where': json.dumps({'Status': 'Active'}), 'order': 'ID', 'limit': 1})
        self.assertEqual([row['objectId'] for row in rows], ['ReRiffServerStatus'])
        response = self.client.get('/classes/News', query_string={'where': json.dumps({'objectId': 'announcement'}), 'count': 1})
        self.assertEqual(response.json['count'], 1)
        self.assertEqual(response.json['results'][0]['enTitle'], 'Normal news')

    def test_game_protocol_version_is_unchanged(self):
        self.assertEqual(self.client.get('/classes/Version').json['results'][0]['Ver'], 100)


if __name__ == '__main__':
    unittest.main()
