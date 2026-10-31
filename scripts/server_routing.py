"""INI configuration and a session-pinned HTTP relay for the game."""
import configparser
import http.client
import ipaddress
import json
from urllib.parse import urlsplit

DEFAULT_CONFIG = '''# Restart start-server.sh after editing.
[server]
# Blank means local-only. Example: https://riff.example.org
external_url =
# Must remain a loopback HTTP address on port 5144 for the patched game DLL.
fallback_url = http://127.0.0.1:5144
timeout_seconds = 5
'''
HOP_HEADERS = {'connection', 'keep-alive', 'proxy-authenticate', 'proxy-authorization',
               'te', 'trailer', 'transfer-encoding', 'upgrade', 'host', 'content-length'}


def load_config(path):
    config = configparser.ConfigParser(interpolation=None)
    config.read_string(DEFAULT_CONFIG)
    if path.exists():
        with path.open() as stream:
            config.read_file(stream)
    section = config['server']
    local = urlsplit(section['fallback_url'].strip())
    if (local.scheme != 'http' or local.hostname not in ('127.0.0.1', 'localhost')
            or local.port != 5144 or local.path not in ('', '/') or local.query
            or local.fragment or local.username or local.password):
        raise ValueError('fallback_url must be http://127.0.0.1:5144 (or localhost:5144).')
    external = section['external_url'].strip().rstrip('/')
    if external:
        parsed = urlsplit(external)
        if (parsed.scheme not in ('http', 'https') or not parsed.hostname or parsed.query
                or parsed.fragment or parsed.username or parsed.password):
            raise ValueError('external_url must be an HTTP(S) URL without credentials, query, or fragment.')
        try:
            loopback = ipaddress.ip_address(parsed.hostname).is_loopback
        except ValueError:
            loopback = parsed.hostname.lower() == 'localhost'
        if loopback and (parsed.port or (443 if parsed.scheme == 'https' else 80)) == 5144:
            raise ValueError('external_url cannot point back to the local relay on port 5144.')
    timeout = section.getfloat('timeout_seconds')
    if not 0 < timeout <= 60:
        raise ValueError('timeout_seconds must be between 0 and 60.')
    return {'external_url': external, 'fallback_url': section['fallback_url'].strip().rstrip('/'),
            'timeout': timeout}


def exchange(base, method, path, body, headers, timeout):
    url = urlsplit(base)
    cls = http.client.HTTPSConnection if url.scheme == 'https' else http.client.HTTPConnection
    connection = cls(url.hostname, url.port, timeout=timeout)
    try:
        connection.request(method, url.path.rstrip('/') + path, body=body, headers=headers)
        response = connection.getresponse()
        return response.status, response.getheaders(), response.read()
    finally:
        connection.close()


def rewrite_file_urls(value, external, local):
    if isinstance(value, dict):
        return {k: rewrite_file_urls(v, external, local) for k, v in value.items()}
    if isinstance(value, list):
        return [rewrite_file_urls(v, external, local) for v in value]
    if isinstance(value, str) and value.startswith(external + '/files/'):
        return local + value[len(external):]
    return value


def configure(app, config):
    """Choose once at startup; never replay failed writes to another database."""
    from flask import Response, g, jsonify, request
    external = config['external_url']
    selected = 'local'
    reason = 'local_only'
    if external:
        try:
            status, _, data = exchange(external, 'GET', '/classes/Version', None,
                                       {'Accept': 'application/json'}, config['timeout'])
            payload = json.loads(data)
            if status != 200 or not isinstance(payload, dict) or not isinstance(payload.get('results'), list):
                raise ValueError('Unexpected health response')
            selected, reason = 'external', 'external_available'
        except (OSError, http.client.HTTPException, ValueError):
            reason = 'external_unavailable_at_startup'
    print(f'Server selection: {selected} ({reason})', flush=True)

    @app.before_request
    def route_backend():
        # The administration panel always manages the local database.
        admin = request.path == '/admin' or request.path.startswith('/admin/')
        g.backend = 'local' if admin else selected
        g.backend_reason = 'local_admin' if admin else reason
        if g.backend != 'external':
            return None
        path = request.path
        if request.query_string:
            path += '?' + request.query_string.decode('ascii')
        headers = {k: v for k, v in request.headers if k.lower() not in HOP_HEADERS}
        headers['Accept-Encoding'] = 'identity'
        body = request.get_data(cache=True)
        if request.is_json:
            try:
                payload = json.loads(body)
                rewritten = rewrite_file_urls(payload, config['fallback_url'], external)
                if rewritten != payload:
                    body = json.dumps(rewritten).encode('utf-8')
            except (ValueError, UnicodeError):
                pass
        try:
            status, response_headers, data = exchange(external, request.method, path,
                                                      body, headers, config['timeout'])
        except (OSError, http.client.HTTPException):
            g.backend_reason = 'external_request_failed_no_retry'
            return jsonify(code=1, error='External server unavailable. Restart the local server to reselect a backend.'), 502
        content_type = next((v for k, v in response_headers if k.lower() == 'content-type'), '')
        changed = False
        if 'json' in content_type:
            try:
                payload = json.loads(data)
                rewritten = rewrite_file_urls(payload, external, config['fallback_url'])
                if rewritten != payload:
                    data = json.dumps(rewritten).encode('utf-8')
                    changed = True
            except (ValueError, UnicodeError):
                pass
        # No automatic redirects/retries: preserve the original response semantics.
        excluded = HOP_HEADERS | ({'etag', 'content-md5'} if changed else set())
        return Response(data, status=status,
                        headers=[(k, v) for k, v in response_headers if k.lower() not in excluded])

    return selected
