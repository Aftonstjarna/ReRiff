"""Fetch independently versioned font components using committed asset pins."""
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PIN = ROOT/'packaging/fonts/release.json'
NAMES = {'cjk': ('cjk-component', 'ReRiff-CJK.zip'),
         'latin': ('latin-component', 'ReRiff-Latin.zip')}


def load_pin(path=PIN):
    pin = json.loads(Path(path).read_text(encoding='utf-8'))
    if pin.get('schema') != 1 or pin.get('repository') != 'Aftonstjarna/ReRiff-Fonts':
        raise ValueError('Unsupported font release pin')
    if not re.fullmatch(r'\d+\.\d+\.\d+', pin.get('version', '')):
        raise ValueError('Invalid independent font version')
    if set(pin.get('packs', {})) != set(NAMES):
        raise ValueError('Pin both independent font packs')
    for pack, entry in pin['packs'].items():
        if entry.get('asset') != NAMES[pack][1] or type(entry.get('size')) is not int or entry['size'] <= 0:
            raise ValueError('Invalid font asset metadata')
        if not re.fullmatch(r'[0-9a-f]{64}', entry.get('sha256', '')):
            raise ValueError('Invalid font asset checksum')
    return pin


def validate_component(path, pin, pack):
    path = Path(path)
    entry = pin['packs'][pack]
    if path.stat().st_size != entry['size']:
        raise ValueError('Font asset size mismatch: '+pack)
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024*1024), b''):
            digest.update(chunk)
    if digest.hexdigest() != entry['sha256']:
        raise ValueError('Font asset checksum mismatch: '+pack)
    with zipfile.ZipFile(path) as archive:
        metadata = json.loads(archive.read('component.json'))
    if metadata.get('schema') != 2 or metadata.get('version') != pin['version']:
        raise ValueError('Font component version/schema differs from its pin: '+pack)
    if metadata.get('component', 'cjk') != pack:
        raise ValueError('Incorrect font component: '+pack)
    return path


def acquire(destination, pin, pack):
    destination = Path(destination)
    if destination.is_file():
        try:
            return validate_component(destination, pin, pack)
        except (ValueError, zipfile.BadZipFile, KeyError):
            pass
    entry = pin['packs'][pack]
    url = ('https://github.com/'+pin['repository']+'/releases/download/v'+
           pin['version']+'/'+entry['asset'])
    request = urllib.request.Request(url, headers={'User-Agent': 'ReRiff-font-assets'})
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=destination.parent, suffix='.download', delete=False) as output:
            temporary = Path(output.name)
            with urllib.request.urlopen(request, timeout=120) as response:
                total = 0
                while True:
                    chunk = response.read(min(1024*1024, entry['size']-total+1))
                    if not chunk:
                        break
                    total += len(chunk)
                    if total > entry['size']:
                        raise ValueError('Font download exceeds its pinned size: '+pack)
                    output.write(chunk)
        validate_component(temporary, pin, pack)
        os.replace(temporary, destination)
        return destination
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def fetch_all(root=ROOT):
    pin = load_pin()
    for pack, (directory, filename) in NAMES.items():
        path = acquire(Path(root)/'build'/directory/filename, pin, pack)
        print('Verified font pack', pin['version'], pack, path)
