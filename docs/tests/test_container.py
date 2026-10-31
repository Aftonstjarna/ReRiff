"""Test the real image with disposable storage and no dependency on port 5144."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import urllib.request
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', default='docker')
    parser.add_argument('--image', default='riffapi-local:test')
    args = parser.parse_args()
    name = 'riffapi-container-test-'+uuid.uuid4().hex[:10]
    volume = name+'-data'
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

    def engine(*command):
        return subprocess.check_output([args.engine, *command], text=True).strip()

    def request(path, method='GET', data=None, mime='application/json'):
        if isinstance(data, dict):
            data = json.dumps(data).encode()
        req = urllib.request.Request(base+path, data=data, method=method, headers={'Content-Type': mime})
        with opener.open(req, timeout=5) as response:
            return response.status, response.read()

    def start():
        engine('run', '-d', '--name', name, '--read-only', '--tmpfs', '/tmp',
               '--cap-drop=all', '--security-opt=no-new-privileges',
               '-p', '127.0.0.1::5144', '-v', volume+':/data', args.image)
        port = engine('port', name, '5144/tcp').splitlines()[0].rsplit(':', 1)[1]
        return 'http://127.0.0.1:'+port

    def ready():
        deadline = time.monotonic()+30
        while time.monotonic() < deadline:
            try:
                if json.loads(request('/')[1]).get('dataDir') == '/data':
                    return
            except (OSError, ValueError):
                pass
            time.sleep(0.1)
        raise RuntimeError('Container API did not become reachable through its host port')

    engine('volume', 'create', volume)
    try:
        base = start()
        ready()
        assert engine('exec', name, 'id', '-u') == '10001'
        engine('exec', name, 'python', '/app/docker/healthcheck.py')
        assert request('/admin')[0] == 200
        environment = dict(os.environ, RIFFAPI_TEST_URL=base)
        tests = Path(__file__).resolve().parent
        for suite in ('test_api.py', 'test_auth.py', 'test_ops.py'):
            subprocess.run([sys.executable, str(tests/suite)], env=environment, check=True)
        object_id = json.loads(request('/classes/Score', 'POST', {'Score': 12345, 'SongID': name})[1])['objectId']
        binary = b'\x00\xffcontainer-persistence'*128
        uploaded = json.loads(request('/files/container.ghost', 'POST', binary, 'application/octet-stream')[1])
        assert uploaded['url'] == base+'/files/container.ghost'
        assert request('/files/container.ghost')[1] == binary
        # Recreating, rather than merely restarting, catches writable-layer data loss.
        engine('stop', '--time', '10', name)
        engine('rm', name)
        base = start()
        ready()
        assert json.loads(request('/classes/Score/'+object_id)[1])['Score'] == 12345
        assert request('/files/container.ghost')[1] == binary
        engine('exec', name, 'python', '/app/docker/healthcheck.py')
        print('PASS: reachable published port, non-root/read-only writes, admin/health, existing API suites, and records/files after container recreation')
    except Exception:
        subprocess.run([args.engine, 'logs', name], check=False)
        raise
    finally:
        subprocess.run([args.engine, 'rm', '-f', name], stdout=subprocess.DEVNULL, check=False)
        subprocess.run([args.engine, 'volume', 'rm', volume], stdout=subprocess.DEVNULL, check=False)


if __name__ == '__main__':
    main()
