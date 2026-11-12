"""Fetch the pinned upstream font collection for native atlas generation."""
import argparse
import hashlib
import json
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def fetch(directory):
    manifest = json.loads((ROOT/'packaging/fonts/noto-cjk.json').read_text())
    directory.mkdir(parents=True, exist_ok=True)
    for entry in manifest['files']:
        destination = directory/Path(entry['path']).name
        def verified(path):
            return path.is_file() and path.stat().st_size == entry['size'] and hashlib.sha256(path.read_bytes()).hexdigest() == entry['sha256']
        if not verified(destination):
            request = urllib.request.Request('https://raw.githubusercontent.com/notofonts/noto-cjk/'
                +manifest['commit']+'/'+entry['path'], headers={'User-Agent': 'ReRiff-font-builder'})
            with urllib.request.urlopen(request, timeout=60) as response:
                data = response.read(entry['size']+1)
            if len(data) != entry['size'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
                raise ValueError('Noto CJK source checksum mismatch: '+entry['path'])
            temporary = destination.with_name(destination.name+'.download')
            temporary.write_bytes(data)
            temporary.replace(destination)
        print('Verified', destination.name)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=ROOT/'build/noto-cjk')
    fetch(parser.parse_args().directory)
