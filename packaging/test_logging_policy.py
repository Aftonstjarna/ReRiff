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

class TimestampTests(unittest.TestCase):
    def test_identity_header_is_allowlisted_and_preserves_player_unicode(self):
        import io
        from logging_policy import TimestampWriter
        with tempfile.TemporaryDirectory() as tmp:
            game=Path(tmp);(game/'localserver').mkdir()
            (game/'localserver/diagnostic-context.ini').write_text('[diagnostic]\nplayer=日本語\naccount_id=22222222-2222-2222-2222-222222222222\nserver_id=11111111-1111-1111-1111-111111111111\ntoken=DO-NOT-LOG\n')
            sink=io.StringIO();writer=TimestampWriter(sink,game);writer.write('first\nsecond\n');writer.flush()
            text=sink.getvalue()
            self.assertEqual(text.count('[diagnostic]'),1)
            self.assertIn('player=日本語',text.splitlines()[0])
            self.assertIn('account_id=22222222-2222-2222-2222-222222222222',text)
            self.assertNotIn('DO-NOT-LOG',text)
            self.assertIn('identity=last-known',text)
            from logging_policy import diagnostic_header
            import uuid
            client=uuid.UUID((game/'localserver/diagnostic-client-id.txt').read_text())
            self.assertIn('client_id='+str(client),diagnostic_header(game))
            missing=game/'uninstalled'
            diagnostic_header(missing)
            self.assertFalse(missing.exists())

    def test_multiline_fragments_flush_and_utf8(self):
        import io
        from logging_policy import TimestampWriter, timestamp_text
        sink=io.StringIO();writer=TimestampWriter(sink)
        writer.write('frag');writer.flush();writer.write('ment\r');writer.write('\n日本語\n  traceback\n');writer.flush()
        lines=sink.getvalue().splitlines()
        self.assertEqual([line[27:] for line in lines],['fragment','日本語','  traceback'])
        for line in lines:self.assertRegex(line,r'^\[\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z\] ')
        self.assertEqual(timestamp_text(lines[0]+'\n'),lines[0]+'\n')
        writer.write('tail');writer.flush();self.assertTrue(sink.getvalue().endswith('tail'))

if __name__=='__main__':unittest.main()
