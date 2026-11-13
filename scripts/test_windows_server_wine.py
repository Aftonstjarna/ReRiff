"""Exercise the private Windows Flask server under Wine without Podman or pip."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--wine', required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--launcher-test', type=Path, help='Native launcher Windows-control regression executable')
    parser.add_argument('--python', type=Path, required=True, help='Unpacked official Windows embedded Python')
    args = parser.parse_args()
    asset = ROOT/'build/windows-server/ReRiff-Server-windows.zip'
    digest = hashlib.sha256(asset.read_bytes()).hexdigest()
    env = dict(os.environ, WINEPREFIX=str(args.prefix.resolve()), WINEDEBUG='-all', WINEDLLOVERRIDES='mscoree,mshtml=')
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    with tempfile.TemporaryDirectory(prefix='windows-flask-test-', dir=ROOT/'build') as temporary:
        local = Path(temporary)/'Game ü with spaces/localserver'
        local.mkdir(parents=True)
        shutil.copytree(args.python, local/'windows-python')
        for name in ('local_server.py','host_process.py','windows_package.py'):
            shutil.copy2(ROOT/'LinuxRuntime'/name, local/name)
        shutil.copy2(ROOT/'scripts/windows-server-control.py', local/'windows-server-control.py')
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0));port = sock.getsockname()[1]
        (local/'native').mkdir()
        (local/'native/config.json').write_text(json.dumps({'schema':1, 'port':port}))
        url = 'http://127.0.0.1:'+str(port)
        def win(path):
            return 'Z:'+str(path.resolve()).replace('/', '\\')
        def action(name, extra=(), success=True):
            result = local/'result.ini'
            result.unlink(missing_ok=True)
            command = [args.wine, str(local/'windows-python/python.exe'), '-I', win(local/'windows-server-control.py'), name,
                       '--root', win(local), '--result', win(result), *extra]
            # Wine descendants keep Unix pipe descriptors alive even when their
            # Windows stdout is redirected. Use a file instead of communicate().
            with (local/'wine-test.log').open('ab') as log:
                run = subprocess.run(command, env=env, stdout=log, stderr=log, timeout=90)
            if not result.exists():
                raise AssertionError(f'{name}: {run.returncode}: '+(local/'wine-test.log').read_text(errors='replace')[-2000:])
            config = configparser.ConfigParser();config.read(result, encoding='utf-16')
            response = dict(config['result'])
            assert (response['ok']=='true') == success, (name, response)
            print(name, response['state'], response.get('message',''), flush=True)
            return response
        try:
            assert action('status')['state'] == 'uninstalled'
            action('install', ('--asset',win(asset),'--sha256','0'*64), success=False)
            assert action('install', ('--asset',win(asset),'--sha256',digest))['state'] == 'stopped'
            if args.launcher_test:
                with (local/'wine-test.log').open('ab') as log:
                    subprocess.run([args.wine, str(args.launcher_test.resolve()), win(local.parent)], env=env, stdout=log, stderr=log, check=True, timeout=180)
                print('Native launcher Windows control passed.', flush=True)
            assert action('start')['state'] == 'running'
            original = json.loads((local/'native/process.json').read_text())
            assert action('start')['state'] == 'running'
            assert json.loads((local/'native/process.json').read_text()) == original
            with opener.open(url+'/classes/Version',timeout=5) as response:
                assert response.status == 200
                json.load(response)
            try:
                opener.open(urllib.request.Request(url+'/reriff-local/stop', data=b''),timeout=5)
                raise AssertionError('Unauthenticated stop was accepted')
            except urllib.error.HTTPError as error:
                assert error.code == 403
            data = local/'data/persistence-check.txt';data.write_text('retained')
            assert action('stop')['state'] == 'stopped'
            assert not (local/'native/process.json').exists()
            action('stop')
            action('start');assert data.read_text() == 'retained';action('stop')
            with socket.socket() as foreign:
                # Use a fresh port rather than a just-closed TCP TIME_WAIT tuple.
                foreign.bind(('127.0.0.1',0));foreign.listen()
                blocked = foreign.getsockname()[1]
                (local/'native/config.json').write_text(json.dumps({'schema':1,'port':blocked}))
                action('start', success=False)
                action('stop')
                assert foreign.getsockname()[1] == blocked
            assert data.read_text() == 'retained'
            print('Windows private Flask lifecycle passed.', flush=True)
        finally:
            action('stop')


if __name__ == '__main__':
    main()
