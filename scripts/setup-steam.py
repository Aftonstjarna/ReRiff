#!/usr/bin/env python3
"""Set up riffapi-local-extended for Steam Riff Racer. Close the game first."""
import argparse
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

OLD = b'https://parseapi.back4app.com'
NEW = b'http://127.000000000.0.1:5144'
REV = 'd6b98d2845bfbc8ee2888f805f8f0dc36bb73ede'


def patch(data):
    if data.count(OLD) == 1 and NEW not in data:
        return data.replace(OLD, NEW, 1)
    if data.count(NEW) == 1 and OLD not in data:
        return data
    raise ValueError('Unrecognized DLL endpoint; refusing to modify it.')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game-dir', type=Path, default=Path.home() / '.local/share/Steam/steamapps/common/Riff Racer')
    p.add_argument('--install-dir', type=Path, help='Defaults to localserver inside the game directory')
    p.add_argument('--check', action='store_true', help='Read-only inspection; no downloads')
    p.add_argument('--restore', action='store_true', help='Restore DLL backup; keep saved data')
    args = p.parse_args()
    game = args.game_dir.expanduser().resolve()
    dest = (args.install_dir or game/'localserver').expanduser().resolve()
    dll = game / 'ParseOctane.dll'
    backup = game / 'ParseOctane.dll.reriff-original'
    if not (game / 'Game.exe').is_file():
        raise ValueError(f'Game.exe missing in {game}')
    current = dll.read_bytes()
    direct_config = b'ReRiff.ClientConfig' in current
    patched = current if direct_config else patch(current)
    if args.check:
        print(f'Game: {game}\nServer: {dest}\nDLL SHA256: {hashlib.sha256(current).hexdigest()}')
        print('Endpoint: ' + ('client-side server.ini reader' if direct_config else 'legacy patch; run setup to upgrade'))
        return
    if args.restore:
        if (game/'ParseOctane.dll.reriff-client-original').exists() or (Path(__file__).resolve().parent.parent/'backups/client/ParseOctane.dll').exists():
            subprocess.run([sys.executable,str(Path(__file__).with_name('install-client-config.py')),
                            '--game-dir',str(game),'--restore'],check=True)
            return
        original = backup.read_bytes()
        if original.count(OLD) != 1 or current not in (original, patch(original)):
            raise ValueError('Backup does not match DLL; refusing to overwrite.')
        dll.write_bytes(original)
        print('Original DLL restored; server and saved data retained.')
        return
    artifacts = Path(__file__).resolve().parent.parent/'build'
    if any(not (artifacts/name).is_file() for name in ('ParseOctane.dll','Parse.NetFx45.dll','ReRiff.ClientConfig.dll','ReRiff.Notify.dll','ReRiff-Launcher.exe')):
        raise ValueError('Build the client patch and Windows launcher first.')
    dest.mkdir(parents=True, exist_ok=True)
    repo, venv = dest / 'server', dest / 'venv'
    if not repo.exists():
        subprocess.run(['git', 'clone', 'https://github.com/rumbadebu/riffapi-local-extended.git', str(repo)], check=True)
        subprocess.run(['git', '-C', str(repo), 'checkout', '--detach', REV], check=True)
    head = subprocess.check_output(['git', '-C', str(repo), 'rev-parse', 'HEAD'], text=True).strip()
    if head != REV:
        raise ValueError('Existing server has another revision; refusing to replace it.')
    env = dict(os.environ, PYTHONNOUSERSITE='1')
    for key in ('PYTHONPATH', 'PYTHONHOME'):
        env.pop(key, None)
    if not (venv / 'bin/python').exists():
        subprocess.run([sys.executable, '-m', 'venv', str(venv)], check=True, env=env)
    python = venv / 'bin/python'
    subprocess.run([str(python), '-m', 'pip', 'install', 'Flask==3.1.2'], check=True, env=env)
    subprocess.run([str(python), '-c', 'import flask'], check=True, env=env)
    launcher = dest / 'start-server.sh'
    tracer = Path(__file__).with_name('trace-server.py')
    if not tracer.is_file():
        raise ValueError('trace-server.py must be beside this setup script.')
    shutil.copy2(tracer, dest / 'trace-server.py')
    routing = Path(__file__).with_name('server_routing.py')
    shutil.copy2(routing, dest / 'server_routing.py')
    config = game / 'server.ini'
    if not config.exists():
        config.write_text('[server]\nexternal_url =\nfallback_url =\ntimeout_seconds = 5\n')
    launcher.write_text('#!/bin/sh\nset -eu\nunset PYTHONPATH PYTHONHOME\nexport PYTHONNOUSERSITE=1\ncd ' + shlex.quote(str(repo)) + '\nexec ' + shlex.quote(str(python)) + ' ' + shlex.quote(str(dest / 'trace-server.py')) + ' --server-dir ' + shlex.quote(str(repo)) + ' --config ' + shlex.quote(str(config)) + '\n')
    launcher.chmod(0o755)
    for name in ('steam-launcher.sh', 'start-server-host.sh'):
        shutil.copy2(Path(__file__).with_name(name), dest/name)
        (dest/name).chmod(0o755)
    shutil.copy2(artifacts/'ReRiff-Launcher.exe',game/'ReRiff-Launcher.exe')
    subprocess.run([sys.executable,str(Path(__file__).with_name('install-client-config.py')),
                    '--game-dir',str(game)],check=True)
    print(f'Setup complete.\nWindows launcher: {game / "ReRiff-Launcher.exe"}\nAdmin: http://127.0.0.1:5144/admin\nData: {repo / "data"}\nBackups are in the ReRiff project backups directory.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(f'Setup failed: {error}')
