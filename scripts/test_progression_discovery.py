#!/usr/bin/env python3
"""Verify client discovery against legacy, preview-only and older prestige APIs."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import subprocess
import threading


class LegacyServer(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        status, body = 404, {}
        if self.path == '/preview/reriff/balance':
            status, body = 200, {'capabilities': {'policyPreview': True}}
        elif self.path == '/old-prestige/reriff/balance':
            status, body = 200, {'capabilities': {'serverCheckpoints': True}}
        payload = json.dumps(body).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client-command', nargs='+', required=True)
    args = parser.parse_args()
    with ThreadingHTTPServer(('127.0.0.1', 0), LegacyServer) as server:
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            subprocess.run([*args.client_command, 'http://127.0.0.1:'+str(server.server_port)], check=True)
        finally:
            server.shutdown()
            thread.join(timeout=5)


if __name__ == '__main__':
    main()
