"""Quick-deploy LAN data sharing and host-only control boundaries."""
import importlib.util
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import unittest
import urllib.request
import urllib.error
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'LinuxRuntime'))
import local_server
import serve


class LocalServerLanTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.stage = Path(self.temp.name)
        (self.stage/'server').symlink_to(ROOT/'vendor/riffapi-local-extended', target_is_directory=True)
        (self.stage/'trace-server.py').symlink_to(ROOT/'scripts/trace-server.py')
        spec = importlib.util.spec_from_file_location('quick_deploy_backend', ROOT/'docker/server.py')
        self.backend = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.backend)
        self.backend.ROOT = self.stage
        self.local = local_server.LocalServer(self.stage/'game/localserver')
        self.local.logging = False

    def app(self):
        def factory(**options):
            return self.backend.create_app(data_dir=self.local.data, **options)
        with patch.object(serve, 'lan_addresses', return_value=['http://192.0.2.10:5144']), patch.dict(os.environ, {'RERIFF_MIGRATE_FROM_ORIGIN': ''}):
            return serve.create_lan_app(factory, self.local)

    def test_lan_and_loopback_share_records_and_reachable_file_links(self):
        client = self.app().test_client()
        hosts = ['127.0.0.1:5144', '192.0.2.10:5144']
        uploaded = client.post('/files/ghost.bin', data=b'ghost payload', headers={'Host': hosts[0]})
        self.assertEqual(uploaded.status_code, 201)
        obj = {'objectId': 'same-song', 'data': {'__type': 'File', 'name': 'ghost.bin', 'url': uploaded.json['url']}}
        path = self.local.data/'classes/FileData/same-song.json'
        path.parent.mkdir(exist_ok=True)
        path.write_text(json.dumps(obj))
        before = path.read_bytes()
        ids = []
        for host in hosts:
            response = client.get('/classes/FileData/same-song', headers={'Host': host})
            self.assertEqual(response.json['data']['url'], 'http://'+host+'/files/ghost.bin')
            ids.append(response.headers['X-ReRiff-Server-ID'])
            self.assertEqual(client.get('/files/ghost.bin', headers={'Host': host}).data, b'ghost payload')
        self.assertEqual(ids[0], ids[1])
        self.assertEqual(path.read_bytes(), before)

    def test_lan_cannot_read_control_token_stop_server_or_admin(self):
        app = self.app()
        app.add_url_rule('/reriff-local/status', 'probe_status', lambda: {'token': 'private-token'})
        app.add_url_rule('/reriff-local/stop', 'probe_stop', lambda: {'stopping': True}, methods=['POST'])
        client = app.test_client()
        for endpoint in ['/reriff-local/status', '/reriff-local/stop', '/admin', '/admin/api/overview']:
            with self.subTest(endpoint=endpoint):
                response = client.open(endpoint, method='POST' if endpoint.endswith('/stop') else 'GET',
                    headers={'Host': '127.0.0.1:5144', 'X-Forwarded-For': '127.0.0.1'},
                    environ_overrides={'REMOTE_ADDR': '192.0.2.20'})
                self.assertEqual(response.status_code, 403)
        self.assertEqual(client.get('/reriff-local/status').json, {'token': 'private-token'})
        with client.get('/admin') as response:
            self.assertEqual(response.status_code, 200)
        self.assertEqual(client.get('/classes/Version', environ_overrides={'REMOTE_ADDR': '192.0.2.20'}).status_code, 200)

    def test_no_interface_discovery_does_not_block_localhost(self):
        with patch.object(local_server.socket, 'getaddrinfo', side_effect=OSError), patch.object(local_server.socket, 'socket', side_effect=OSError):
            self.assertEqual(local_server.lan_addresses(5144), [])
        self.assertEqual(self.local.address, 'http://127.0.0.1:5144')

    def test_real_listener_accepts_non_loopback_and_stops_cleanly(self):
        addresses = local_server.lan_addresses(5144)
        if not addresses:
            self.skipTest('No LAN interface available')
        import shutil
        for name in ['serve.py', 'local_server.py', 'host_process.py']:
            shutil.copy2(ROOT/'LinuxRuntime'/name, self.stage/name)
        (self.stage/'docker').mkdir()
        shutil.copy2(ROOT/'docker/server.py', self.stage/'docker/server.py')
        with socket.socket() as reservation:
            reservation.bind(('0.0.0.0', 0))
            port = reservation.getsockname()[1]
        control = self.local.control
        control.mkdir(parents=True)
        (control/'config.json').write_text(json.dumps({'schema': 1, 'port': port}))
        env = dict(os.environ, RERIFF_LOCAL_TOKEN='1'*64, RERIFF_MIGRATE_FROM_ORIGIN='')
        with (self.stage/'child.log').open('w+') as log:
            process = subprocess.Popen([sys.executable, str(self.stage/'serve.py'), '--root', str(self.local.root)], env=env, stdout=log, stderr=log)
            try:
                deadline = time.monotonic()+15
                while not (control/'process.json').exists():
                    if process.poll() is not None or time.monotonic()>deadline:
                        log.seek(0)
                        self.fail(log.read())
                    time.sleep(.05)
                address = addresses[0].rsplit(':', 1)[0]+':'+str(port)
                opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
                with opener.open(address+'/classes/Version', timeout=5) as response:
                    self.assertEqual(response.status, 200)
                with self.assertRaises(urllib.error.HTTPError) as error:
                    opener.open(address+'/reriff-local/status', timeout=5)
                self.assertEqual(error.exception.code, 403)
                error.exception.close()
            finally:
                process.terminate()
                process.wait(timeout=10)
            self.assertFalse((control/'process.json').exists())


if __name__ == '__main__':
    unittest.main()
