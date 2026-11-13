"""Run NativeLauncher/test_linux_server.c against real AppImages under Wine."""
import argparse
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--wine', required=True)
    parser.add_argument('--prefix', type=Path, required=True, help='Use an isolated test prefix')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='wine-local-server-', dir=ROOT/'build') as temporary:
        game = Path(temporary)/'Game ü with spaces'
        local = game/'localserver'
        (local/'native').mkdir(parents=True)
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0));port = sock.getsockname()[1]
        (local/'native/config.json').write_text(json.dumps({'schema': 1, 'port': port}))
        shutil.copy2(ROOT/'build/linux-patcher/Patcher', local/'ReRiff-ClientHost')
        shutil.copy2(ROOT/'build/linux-runtime/ReRiff-Server-x86_64.AppImage', local/'ReRiff-Server-x86_64.AppImage')
        shutil.copy2(ROOT/'scripts/linux-runtime.sh', local/'linux-runtime.sh')
        args.prefix.mkdir(parents=True, exist_ok=True)
        env = dict(os.environ, WINEPREFIX=str(args.prefix.resolve()), WINEDEBUG='-all',
                   WINEDLLOVERRIDES='mscoree,mshtml=')
        try:
            subprocess.run(['xvfb-run', '-a', args.wine, str(ROOT/'build/test-linux-server.exe'),
                            'Z:'+str(game)], env=env, check=True, timeout=180)
        finally:
            subprocess.run([str(local/'ReRiff-ClientHost'), '--client-host', 'server-control',
                            'stop', '--root', str(local), '--result', str(local/'cleanup.ini')],
                           check=True, timeout=40)


if __name__ == '__main__':
    main()
