import tempfile
import unittest
from pathlib import Path
from logging_policy import logging_enabled, archive_grouped_logs

class GroupedLoggingTests(unittest.TestCase):
    def test_archive_preserves_bytes_and_active_group(self):
        with tempfile.TemporaryDirectory() as tmp:
            game=Path(tmp)
            self.assertEqual(archive_grouped_logs(game),{})
            self.assertFalse((game/'logs').exists())
            (game/'logs').mkdir()
            (game/'logs/ReRiff-network.log').write_bytes(b'old\x00\xff')
            (game/'logs/ReRiff-connections.log').write_bytes(b'current')
            moved=archive_grouped_logs(game)
            self.assertEqual(len(moved),1)
            self.assertEqual(Path(next(iter(moved.values()))).read_bytes(),b'old\x00\xff')
            self.assertEqual((game/'logs/ReRiff-connections.log').read_bytes(),b'current')
            self.assertEqual(archive_grouped_logs(game),{})

    def test_group_override_and_legacy_fallback(self):
        with tempfile.TemporaryDirectory() as tmp:
            game=Path(tmp)
            (game/'client.ini').write_text('[diagnostics]\nlogging_enabled=true\nlog_network=false\nlog_unicode=false\n')
            self.assertFalse(logging_enabled(game,category='network'))
            self.assertTrue(logging_enabled(game,category='news'))
            (game/'client.ini').write_text('[diagnostics]\nlogging_enabled=true\nlog_network=false\nlog_connections=true\nlog_game=false\nlog_text=false\n')
            self.assertTrue(logging_enabled(game,category='network'))
            for category in ('engine_menu','singleplayer','unicode','metadata'):
                self.assertFalse(logging_enabled(game,category=category))
            self.assertFalse((game/'logs').exists())

if __name__=='__main__':unittest.main()
