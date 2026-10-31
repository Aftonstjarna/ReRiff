import json
from pathlib import Path
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from unittest.mock import patch

from flask import Flask
import server_routing as routing


class RoutingTests(unittest.TestCase):
    def config(self, external=''):
        return dict(external_url=external, fallback_url='http://127.0.0.1:5144', timeout=0.2)

    def app(self, config):
        app = Flask(__name__)
        self.writes = []

        @app.route('/classes/Score', methods=['POST'])
        def local_write():
            self.writes.append(True)
            return {'local': True}

        @app.route('/admin')
        def admin():
            return 'local admin'

        routing.configure(app, config)
        return app.test_client()

    def test_config_validation(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'server.ini'
            self.assertEqual(routing.load_config(path)['external_url'], '')
            for bad in ('http://127.0.0.1:5144', 'http://localhost:5144', 'ftp://host', 'https://a:b@host'):
                path.write_text('[server]\nexternal_url = '+bad)
                with self.assertRaises(ValueError):
                    routing.load_config(path)

    def test_unavailable_external_selects_local(self):
        with patch.object(routing, 'exchange', side_effect=OSError('offline')) as exchange:
            client = self.app(self.config('https://example.invalid'))
            self.assertEqual(client.post('/classes/Score', json={'Score': 5}).json, {'local': True})
            self.assertEqual(exchange.call_count, 1)
            self.assertEqual(len(self.writes), 1)

    def test_write_failure_never_retries_locally(self):
        with patch.object(routing, 'exchange', side_effect=[(200, [], b'{"results":[]}'), OSError('lost response')]) as exchange:
            client = self.app(self.config('https://example.invalid'))
            self.assertEqual(client.post('/classes/Score', json={'Score': 5}).status_code, 502)
            self.assertEqual(self.writes, [])
            self.assertEqual(exchange.call_count, 2)
            self.assertEqual(client.get('/admin').data, b'local admin')

    def test_real_http_relay_and_file_rewrite(self):
        calls = []

        class Remote(BaseHTTPRequestHandler):
            def do_GET(self):
                if self.path == '/classes/Version':
                    data, mime = b'{"results":[]}', 'application/json'
                else:
                    data, mime = b'\x00\xffghost', 'application/octet-stream'
                self.send_response(200)
                self.send_header('Content-Type', mime)
                self.end_headers()
                self.wfile.write(data)

            def do_POST(self):
                calls.append((self.path, self.rfile.read(int(self.headers['Content-Length']))))
                data = json.dumps({'url': base+'/files/a.ghost', 'name':'a.ghost'}).encode()
                self.send_response(201)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(data)

            def log_message(self, *args):
                pass

        with ThreadingHTTPServer(('127.0.0.1', 0), Remote) as server:
            base = f'http://127.0.0.1:{server.server_port}'
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                client = self.app(self.config(base))
                response = client.post('/files/a.ghost?test=1', data=b'\x00\xffghost')
                self.assertEqual(response.status_code, 201)
                self.assertEqual(response.json['url'], 'http://127.0.0.1:5144/files/a.ghost')
                self.assertEqual(calls, [('/files/a.ghost?test=1', b'\x00\xffghost')])
                client.post('/classes/FileData', json={'data': {'url': response.json['url']}})
                self.assertEqual(json.loads(calls[-1][1])['data']['url'], base+'/files/a.ghost')
                self.assertEqual(client.get('/files/a.ghost').data, b'\x00\xffghost')
                self.assertEqual(client.get('/admin').data, b'local admin')
            finally:
                server.shutdown()
                thread.join()


if __name__ == '__main__':
    unittest.main()
