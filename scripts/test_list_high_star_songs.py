"""Regression tests for the read-only 4-5 star cache listing helper."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('high_star_songs', ROOT / 'scripts/list-high-star-songs.py')
songs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(songs)


def cache_record(path, song_id, title, artist, rating):
    fields = (path, song_id, title, artist, '', '', '')
    record = bytearray()
    for value in fields:
        record.extend(value.encode('utf-8') + b'\0')
    record.extend(struct.pack('<3f4IB', 0.0, 0.0, 0.0, 0, 0, 0, rating, 1))
    return bytes(record)


def cache_bytes(records):
    return struct.pack('<4I', songs.MAGIC, songs.CACHE_VERSION, len(records), songs.RECORD_VERSION) + b''.join(records)


class HighStarSongsTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)

    def write(self, name, records):
        path = self.folder / name
        path.write_bytes(cache_bytes(records))
        return path

    def test_parses_reads_rating_and_rejects_trailing_bytes(self):
        path = self.write('cache.bin', [
            cache_record('hst:C:/Music/first.flac', 'first-id', 'First', 'Artist', 5),
        ])
        parsed = songs.parse_cache(path)
        self.assertEqual(len(parsed), 1)
        self.assertEqual(parsed[0].rating, 5)
        self.assertEqual(parsed[0].artist, 'Artist')
        self.assertEqual(parsed[0].title, 'First')
        self.assertEqual(parsed[0].path, 'hst:C:/Music/first.flac')
        path.write_bytes(path.read_bytes() + b'extra')
        with self.assertRaises(ValueError):
            songs.parse_cache(path)

    def test_deduplicates_artist_title_and_keeps_five_star(self):
        first = self.write('cache.singleplayer.bin', [
            cache_record('hst:C:/Music/one.flac', 'one', 'Alpha', 'Artist', 4),
            cache_record('hst:C:/Music/beta.flac', 'beta', 'Beta', 'Artist', 3),
        ])
        second = self.write('cache.server.bin', [
            cache_record('hst:D:/Music/one.flac', 'copy', 'Alpha', 'Artist', 5),
            cache_record('hst:D:/Music/gamma.flac', 'gamma', 'Gamma', 'Other', 5),
            cache_record('hst:D:/Music/unnamed.flac', 'unnamed', '', '', 4),
        ])
        merged = songs.parse_cache(first) + songs.parse_cache(second)
        catalog = songs.unique_high_stars(merged)
        self.assertEqual([(song.rating, song.artist, song.title) for song in catalog], [
            (5, 'Artist', 'Alpha'),
            (5, 'Other', 'Gamma'),
            (4, '', 'unnamed.flac'),
        ])
        text = songs.format_catalog(catalog)
        self.assertTrue(text.startswith('# ReRiff 4-5 star songs: 3 unique (2 five-star, 1 four-star)\n'))
        self.assertEqual(text.count('Artist\tAlpha'), 1)
        self.assertNotIn('Beta', text)

    def test_sanitizes_table_fields_and_optional_paths(self):
        path = self.write('cache.bin', [
            cache_record('hst:C:/Music/tab.flac', 'tab', 'Tab\tTitle\nLine', 'A\tB', 5),
        ])
        catalog = songs.unique_high_stars(songs.parse_cache(path))
        text = songs.format_catalog(catalog)
        self.assertIn('5\tA B\tTab Title Line\n', text)
        with_paths = songs.format_catalog(catalog, True)
        self.assertIn('5\tA B\tTab Title Line\thst:C:/Music/tab.flac\n', with_paths)

    def test_discovers_explicit_cache_without_automatic_scan(self):
        path = self.write('cache.bin', [])
        self.assertEqual(songs.discover_caches([str(path)], []), [path.resolve()])

    def test_empty_catalog_is_still_a_valid_list(self):
        path = self.write('cache.bin', [cache_record('hst:C:/Music/three.flac', 'three', 'Three', 'Artist', 3)])
        catalog = songs.unique_high_stars(songs.parse_cache(path))
        self.assertEqual(catalog, [])
        self.assertEqual(songs.format_catalog(catalog),
                         '# ReRiff 4-5 star songs: 0 unique (0 five-star, 0 four-star)\n# stars\tartist\ttitle\n')


if __name__ == '__main__':
    unittest.main()
