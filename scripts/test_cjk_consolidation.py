"""Verify real CJK volume consolidation through both installer transactions."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'packaging'))
import installer

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--host', type=Path, required=True)
parser.add_argument('--old', type=Path, required=True)
parser.add_argument('--asset', type=Path, required=True)
a = parser.parse_args()
host = a.host.resolve()
sha = lambda data: hashlib.sha256(data).hexdigest()

def invoke(game, asset, action='install', *extra):
    return subprocess.run([str(host), '--language-component', action, '--game-dir', str(game),
                           '--asset', str(asset.resolve()), '--sha256', sha(asset.read_bytes()), *extra],
                          check=True, capture_output=True, text=True, timeout=180)

with tempfile.TemporaryDirectory(prefix='cjk-consolidation-', dir=ROOT/'build') as temp:
    root = Path(temp)
    bundle = root/'bundle'; (bundle/'payload').mkdir(parents=True)
    shutil.copy2(a.asset, bundle/'fonts.zip')
    manifest = {'version':'consolidation-test', 'platform':'linux', 'channel':'experimental', 'singleplayer':True,
                'patches':{}, 'required_existing':{}, 'files':{},
                'language_component':{'file':'fonts.zip', 'sha256':sha(a.asset.read_bytes())}}
    (bundle/'manifest.json').write_text(json.dumps(manifest))
    for platform in ('native', 'python'):
        game = root/platform; game.mkdir(); (game/'Game.exe').write_bytes(b'fixture')
        (game/'localserver').mkdir();(game/'localserver/client-channel.ini').write_text('[client]\nchannel=experimental\n')
        (game/'vfs.dat').write_bytes(b'original index'); (game/'vfs.zip').write_bytes(b'original archive')
        (game/'VFS0001.DAT').write_bytes(b'mod index'); (game/'VFS0001.ZIP').write_bytes(b'mod archive')
        invoke(game, a.old)
        old_state = json.loads((game/'localserver/languages/cjk.json').read_text())
        assert old_state['archives'] == {'base':'vfs2', 'hd':'vfs3'}
        before = {p.name:sha(p.read_bytes()) for p in game.glob('vfs*')}
        if platform == 'native':
            command = [str(host), '--client-host', 'install', '--bundle', str(bundle),
                       '--game-dir', str(game), '--no-music-link', '--cjk']
            subprocess.run(command+['--dry-run'], check=True, capture_output=True, timeout=180)
            assert before == {p.name:sha(p.read_bytes()) for p in game.glob('vfs*')}
            subprocess.run(command, check=True, capture_output=True, timeout=180)
        else:
            run = subprocess.run
            def stage(command, **kwargs):
                assert command[1:3] == ['--language-component', 'stage'], command
                return run([str(host), *command[1:]], **kwargs)
            with patch.object(installer.subprocess, 'run', side_effect=stage):
                installer.install(bundle, game, game/'.bak', 'linux', cjk=True, music_link=False)
        state = json.loads((game/'localserver/languages/cjk.json').read_text())
        assert state['archives'] == {'combined':'vfs2'}
        assert not (game/'vfs3.dat').exists() and not (game/'vfs3.zip').exists()
        for name in ('vfs3.dat', 'vfs3.zip'):
            assert (game/'.bak/versions'/(old_state['files'][name]+'-'+name)).is_file()
        assert (game/'vfs.dat').read_bytes() == b'original index'
        assert (game/'vfs.zip').read_bytes() == b'original archive'
        assert (game/'VFS0001.DAT').read_bytes() == b'mod index'
        assert (game/'VFS0001.ZIP').read_bytes() == b'mod archive'
        with zipfile.ZipFile(game/'vfs2.zip') as z:
            names = z.namelist()
            for region in ('jp', 'kr', 'sc', 'tc', 'hk', 'jp64', 'jp96'):
                assert any('/'+region+'/' in name for name in names), region
        print('PASS:', platform, 'full component migration, read-only preflight, obsolete-pair backup and original/mod preservation.', flush=True)
