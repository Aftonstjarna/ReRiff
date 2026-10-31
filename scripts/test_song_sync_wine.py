"""Exercise the native Windows cache reader/hash/upload against a real server.

Run inside development with --wine pointing to Proton Wine; --host-tools uses
distrobox-host-exec for Wine/Xvfb while the server remains in the container.
Only the supplied disposable prefix and temporary fixture files are used.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import threading

from werkzeug.serving import make_server

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--wine', required=True)
parser.add_argument('--prefix', required=True)
parser.add_argument('--host-tools', action='store_true')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='reriff-song-handshake-') as directory:
    temp = Path(directory)
    spec = importlib.util.spec_from_file_location('handshake_server', ROOT/'vendor/riffapi-local-extended/riffapi-local.py')
    server = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(server)
    server.DATA_DIR = str(temp/'data')
    server.CLASSES_DIR = str(temp/'data/classes')
    server.FILES_DIR = str(temp/'data/files')
    server.LOG_FILE = str(temp/'requests.log')
    server.ensure_dirs()
    http = make_server('127.0.0.1', 0, server.app, threaded=True)
    thread = threading.Thread(target=http.serve_forever, daemon=True)
    thread.start()
    try:
        audio = temp/'source.flac'
        audio.write_bytes(b'fixture audio, identical on both clients\x00\xff')
        records = bytearray(struct.pack('<4I', 1128350512, 19, 2, 18))
        for song in ('cache-one', 'cache-two'):
            for text in ('hst:Z:'+str(audio), song, 'A "quoted" song', 'Artist', '', 'rock', 'Player'):
                records.extend(text.encode('utf-8')+b'\0')
            records.extend(struct.pack('<3f4IB', 0.1, 0.4, 113.4, 0, 0, 0, 0, 0))
        cache = temp/'cache.bin'
        cache.write_bytes(records)
        config = temp/'server.ini'
        config.write_text('[server]\nexternal_url=http://127.0.0.1:'+str(http.server_port)+'\nexternal_only=true\n')
        prefix = Path(args.prefix)
        prefix.mkdir(parents=True, exist_ok=True)
        cmd = ['xvfb-run', '-a', 'env', 'WINEPREFIX='+str(prefix), 'WINEDEBUG=-all', args.wine,
               str(ROOT/'build/server-sync/test-song-sync.exe'), 'Z:'+str(cache), 'Z:'+str(config)]
        if args.host_tools:
            cmd.insert(0, '/usr/bin/distrobox-host-exec')
        subprocess.run(cmd, check=True, timeout=90)
        state = json.loads((temp/'data/song-catalog.json').read_text())
        expected = hashlib.sha256(audio.read_bytes()).hexdigest()
        assert set(state['records']) == {'cache-one', 'cache-two'}
        assert all(record['sha256'] == expected for record in state['records'].values())
        assert len(set(state['aliases'].values())) == 1
        assert all(record['title'] == 'A "quoted" song' for record in state['records'].values())
        assert 'source.flac' not in json.dumps(state)
        print('PASS: real Wine cache parsing, SHA256, escaped metadata, HTTP registration, automatic identity grouping')
    finally:
        http.shutdown()
        thread.join()
