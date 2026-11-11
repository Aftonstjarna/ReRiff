"""Exercise shipped AppImages without system Python/Podman or FUSE."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import urllib.request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--server', type=Path, required=True)
    args = parser.parse_args()
    runtime, image = args.runtime.resolve(), args.server.resolve()
    env = dict(os.environ, PATH='/no-system-tools', APPIMAGE_EXTRACT_AND_RUN='1')
    for key in ('LD_PRELOAD', 'LD_LIBRARY_PATH', 'PYTHONPATH', 'PYTHONHOME'):
        env.pop(key, None)
    subprocess.run([str(runtime), '--self-test'], env=env, check=True, timeout=90)
    with tempfile.TemporaryDirectory(prefix='reriff portable ü ') as temporary:
        game = Path(temporary)/'game with spaces'
        root = game/'localserver';control = root/'native';control.mkdir(parents=True)
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0));port = sock.getsockname()[1]
        (control/'config.json').write_text(json.dumps({'schema': 1, 'port': port}))
        address = 'http://127.0.0.1:'+str(port)
        seq = 0
        def action(name, *extra, success=True):
            nonlocal seq
            seq += 1
            result = Path(temporary)/f'result-{seq}.ini'
            run = subprocess.run([str(runtime), '--server-control', name, '--root', str(root),
                                  '--result', str(result), *map(str, extra)], env=env,
                                 capture_output=True, text=True, timeout=100)
            if not result.exists():
                raise AssertionError(run.stdout+run.stderr)
            config = configparser.ConfigParser(interpolation=None)
            config.read_string(result.read_text(encoding='utf-16'))
            reply = dict(config['result'])
            assert (reply['ok'] == 'true') == success, reply
            assert (run.returncode == 0) == success, reply
            return reply
        digest = hashlib.sha256(image.read_bytes()).hexdigest()
        assert action('status')['state'] == 'uninstalled'
        action('install', '--asset', image, '--sha256', '0'*64, success=False)
        assert not (root/image.name).exists()
        assert action('install', '--asset', image, '--sha256', digest)['state'] == 'stopped'
        preserved = root/'data/persistence-check.txt';preserved.parent.mkdir(exist_ok=True)
        preserved.write_text('saved local data')
        try:
            assert action('start')['state'] == 'running'
            before = json.loads((control/'process.json').read_text())
            assert action('start')['state'] == 'running'
            assert json.loads((control/'process.json').read_text()) == before
            opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
            with opener.open(address+'/classes/Version', timeout=5) as response:
                assert isinstance(json.load(response)['results'], list)
            assert (game/'logs/traces').is_dir()
            assert not (root/'data/traces').exists()
            action('install', '--asset', image, '--sha256', digest, success=False)
            assert action('stop')['state'] == 'stopped'
            assert action('stop')['state'] == 'stopped'
            assert action('start')['state'] == 'running'
            assert preserved.read_text() == 'saved local data'
            assert action('stop')['state'] == 'stopped'
            # An unrelated listener must neither be mistaken for our server nor killed.
            with socket.socket() as sock:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                sock.bind(('127.0.0.1', port));sock.listen()
                action('start', success=False)
                assert action('stop')['state'] == 'stopped'
                assert sock.getsockname()[1] == port
        except BaseException:
            log = game/'logs/ReRiff-localserver.log'
            if log.exists():
                print(log.read_text(errors='replace')[-6000:])
            raise
        finally:
            action('stop')
    print('PASS: AppImages, verified install, loopback API, repeat start/stop, persistence and unrelated listener; no system Python/Podman.')


if __name__ == '__main__':
    main()
