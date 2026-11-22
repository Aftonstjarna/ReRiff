"""List unique 4-5 star songs from Riff Racer's v19 song cache.

The cache is read-only. By default the script looks in the standard Windows
and Proton/Steam locations, merges every cache*.bin it finds, and collapses
duplicate artist/title pairs. Pass --cache (or --search-root) when the cache
lives somewhere else.

Examples:
    python scripts/list-high-star-songs.py
    python scripts/list-high-star-songs.py --cache "/path/to/cache.bin"
    python scripts/list-high-star-songs.py --output logs/ReRiff-high-stars.log
"""
import argparse
import glob
import ntpath
import os
from dataclasses import dataclass
from pathlib import Path
import struct
import sys

MAGIC = 1128350512
CACHE_VERSION = 19
RECORD_VERSION = 18
MAX_CACHE_BYTES = 16 * 1024 * 1024
MAX_RECORDS = 10000
TAIL_BYTES = 29
RATING_OFFSET = 24


@dataclass(frozen=True)
class CachedSong:
    rating: int
    artist: str
    title: str
    path: str
    song_id: str
    source: Path


def parse_cache(path):
    """Parse one v19 cache record list without changing the file."""
    try:
        data = path.read_bytes()
    except OSError as error:
        raise ValueError(f"{path}: {error}") from error
    if len(data) < 16 or len(data) > MAX_CACHE_BYTES:
        raise ValueError(f"{path}: invalid cache size")
    magic, version, count, record_version = struct.unpack_from('<4I', data, 0)
    if magic != MAGIC or version != CACHE_VERSION or record_version != RECORD_VERSION:
        raise ValueError(f"{path}: unsupported cache header")
    if count > MAX_RECORDS:
        raise ValueError(f"{path}: implausible record count {count}")
    songs = []
    cursor = 16
    for index in range(count):
        fields = []
        for _ in range(7):
            end = data.find(b'\0', cursor)
            if end < 0:
                raise ValueError(f"{path}: unterminated string in record {index}")
            fields.append(data[cursor:end].decode('utf-8', errors='replace'))
            cursor = end + 1
        if len(data) - cursor < TAIL_BYTES:
            raise ValueError(f"{path}: truncated record {index}")
        rating = struct.unpack_from('<I', data, cursor + RATING_OFFSET)[0]
        cursor += TAIL_BYTES
        songs.append(CachedSong(rating, fields[3], fields[2], fields[0], fields[1], path))
    if cursor != len(data):
        raise ValueError(f"{path}: unexpected trailing data")
    return songs


def normalized(value):
    return ' '.join(value.split()).casefold()


def fallback_title(path):
    text = path[4:] if path.startswith('hst:') else path
    return ntpath.basename(text.replace('/', '\\')) or text


def unique_high_stars(songs):
    """Keep one entry per normalized artist/title, preferring 5 over 4."""
    chosen = {}
    for song in songs:
        if song.rating not in (4, 5):
            continue
        artist = song.artist.strip()
        title = song.title.strip()
        if not artist and not title:
            title = fallback_title(song.path).strip()
        key = (normalized(artist), normalized(title))
        previous = chosen.get(key)
        if previous is None or song.rating > previous.rating:
            chosen[key] = CachedSong(song.rating, artist, title, song.path, song.song_id, song.source)
    return sorted(chosen.values(), key=lambda song: (
        -song.rating, normalized(song.artist), normalized(song.title), normalized(song.path)))


def field(value):
    return ''.join(' ' if ord(char) < 32 or ord(char) == 127 else char for char in value)


def format_catalog(songs, with_paths=False):
    five = sum(song.rating == 5 for song in songs)
    four = len(songs) - five
    lines = [f"# ReRiff 4-5 star songs: {len(songs)} unique ({five} five-star, {four} four-star)"]
    lines.append('# stars\tartist\ttitle' + ('\tpath' if with_paths else ''))
    for song in songs:
        row = [str(song.rating), field(song.artist), field(song.title)]
        if with_paths:
            row.append(field(song.path))
        lines.append('\t'.join(row))
    return '\n'.join(lines) + '\n'


def cache_patterns():
    home = Path.home()
    patterns = []
    for variable in ('APPDATA', 'LOCALAPPDATA'):
        value = os.environ.get(variable)
        if value:
            patterns.append(Path(value) / 'Octane/DAT/cache*.bin')
    patterns.extend([
        home / '.local/share/Steam/steamapps/compatdata/*/pfx/drive_c/users/*/AppData/Roaming/Octane/DAT/cache*.bin',
        home / '.steam/steam/steamapps/compatdata/*/pfx/drive_c/users/*/AppData/Roaming/Octane/DAT/cache*.bin',
        home / '.var/app/com.valvesoftware.Steam/data/Steam/steamapps/compatdata/*/pfx/drive_c/users/*/AppData/Roaming/Octane/DAT/cache*.bin',
        home / 'Saves/Heroic/Prefixes/*/drive_c/users/*/AppData/Roaming/Octane/DAT/cache*.bin',
        home / 'Games/Heroic/Prefixes/*/drive_c/users/*/AppData/Roaming/Octane/DAT/cache*.bin',
        home / 'Games/*/drive_c/users/*/AppData/Roaming/Octane/DAT/cache*.bin',
    ])
    return patterns


def discover_caches(explicit, search_roots):
    candidates = []
    if explicit:
        candidates.extend(Path(value).expanduser() for value in explicit)
    else:
        value = os.environ.get('RERIFF_CACHE')
        if value:
            candidates.extend(Path(item).expanduser() for item in value.split(os.pathsep) if item)
        for pattern in cache_patterns():
            candidates.extend(Path(value) for value in glob.glob(str(pattern)))
        for root in search_roots:
            candidates.extend(Path(root).expanduser().rglob('cache*.bin'))
    result = []
    seen = set()
    for candidate in candidates:
        try:
            resolved = candidate.resolve()
        except OSError:
            continue
        if resolved in seen or not resolved.is_file():
            continue
        seen.add(resolved)
        result.append(resolved)
    return sorted(result)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--cache', action='append', default=[], metavar='PATH',
                        help='cache file to read; repeatable. Disables automatic discovery.')
    parser.add_argument('--search-root', action='append', default=[], metavar='PATH',
                        help='recursively look for cache*.bin below this directory; repeatable.')
    parser.add_argument('--output', '-o', metavar='PATH',
                        help='write the catalog here instead of standard output.')
    parser.add_argument('--with-paths', action='store_true',
                        help='add the original cached file path as a fourth column.')
    args = parser.parse_args(argv)

    caches = discover_caches(args.cache, args.search_root)
    if not caches:
        parser.error('no cache*.bin found; pass --cache PATH or --search-root PATH')

    songs = []
    errors = []
    for cache in caches:
        try:
            songs.extend(parse_cache(cache))
        except ValueError as error:
            errors.append(str(error))
    for error in errors:
        print('warning: ' + error, file=sys.stderr)
    if not songs and errors:
        return 1

    catalog = unique_high_stars(songs)
    text = format_catalog(catalog, args.with_paths)
    if args.output:
        output = Path(args.output).expanduser()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text, encoding='utf-8', newline='\n')
        print(f'{len(catalog)} unique 4-5 star songs -> {output}', file=sys.stderr)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
