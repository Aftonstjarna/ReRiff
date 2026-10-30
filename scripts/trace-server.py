#!/usr/bin/env python3
"""Run the extended server with redacted JSONL traces and gap candidates."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
from pathlib import Path
import threading
import time
import traceback
import uuid

SECRET = ('password', 'token', 'authorization', 'cookie', 'key', 'appid', 'application-id')
QUERY_OPS = {'$ne', '$in', '$nin', '$exists', '$lt', '$lte', '$gt', '$gte', '$regex', '$options', '$all'}
FIELD_OPS = {'Increment', 'Add', 'AddUnique', 'Remove', 'Delete'}


def redact(value):
    if isinstance(value, dict):
        return {k: '[redacted]' if any(s in k.lower() for s in SECRET) else redact(v) for k, v in value.items()}
    if isinstance(value, list):
        return [redact(v) for v in value]
    return value


def operators(value):
    found = []
    if isinstance(value, dict):
        for k, v in value.items():
            if k.startswith('$') and k not in QUERY_OPS:
                found.append('unsupported_query_operator:' + k)
            if k == '__op' and v not in FIELD_OPS:
                found.append('unsupported_field_operation:' + str(v))
            found.extend(operators(v))
    elif isinstance(value, list):
        for v in value:
            found.extend(operators(v))
    return found


def body_info(data, content_type):
    info = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    if 'json' in content_type:
        try:
            info['json'] = redact(json.loads(data))
        except (ValueError, UnicodeError):
            info['invalid_json'] = True
    # Binary contents stay in the server file store, not duplicated in logs.
    return info


def instrument(server, output):
    from flask import g, request
    from flask.signals import got_request_exception
    output.mkdir(parents=True, exist_ok=True)
    session = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S') + '-' + uuid.uuid4().hex[:8]
    logfile = output / (session + '.jsonl')
    lock = threading.Lock()
    app = server.app
    # Replace upstream request logging, which writes credentials verbatim.
    for name in ('before_request_funcs', 'after_request_funcs'):
        hooks = getattr(app, name)
        for scope in hooks:
            hooks[scope] = [f for f in hooks[scope] if f.__name__ not in ('_log_request', '_log_response')]
    server.log_line = lambda *_: None

    def write(record):
        with lock:
            with logfile.open('a', encoding='utf-8') as stream:
                stream.write(json.dumps(record, ensure_ascii=False) + '\n')

    @app.before_request
    def begin():
        g.trace_start = time.monotonic()
        g.trace_id = uuid.uuid4().hex
        g.trace_error = None
        request.get_data(cache=True)

    def exception(sender, exception, **extra):
        # Stack frames identify failing code without exposing exception values.
        g.trace_error = {'type': type(exception).__name__, 'frames': traceback.format_tb(exception.__traceback__)}

    got_request_exception.connect(exception, app, weak=False)

    @app.after_request
    def finish(response):
        query = request.args.to_dict(flat=False)
        parsed = {}
        for k, values in query.items():
            if k == 'where':
                try:
                    parsed[k] = json.loads(values[0])
                except ValueError:
                    parsed[k] = values
            else:
                parsed[k] = values
        payload = request.get_json(silent=True)
        external = getattr(g, 'backend', 'local') == 'external'
        flags = [] if external else operators(parsed) + operators(payload)
        streamed = response.direct_passthrough or response.is_streamed
        result = response.get_json(silent=True) if not streamed else None
        response_body = ({'bytes': response.content_length, 'streamed': True,
                          'body_omitted': 'preserve_stream'} if streamed else
                         body_info(response.get_data(), response.content_type or ''))
        if request.url_rule is None and not external:
            flags.append('unmatched_route')
        if response.status_code >= 400:
            flags.append('http_error:' + str(response.status_code))
        if isinstance(result, dict) and 'error' in result:
            flags.append('parse_error')
        if isinstance(result, list) and any(isinstance(x, dict) and 'error' in x for x in result):
            flags.append('batch_error')
        if isinstance(result, dict) and result.get('results') == []:
            flags.append('empty_results_review')
        if request.path in ('/login', '/logout') and not external:
            flags.append('local_auth_stub')
        if request.path.startswith('/classes/') and not external:
            cls = request.view_args.get('cls') if request.view_args else None
            if cls in getattr(server, 'CANNED_CLASSES', ()):
                flags.append('canned_response')
            if cls == 'Profile_Steam' and isinstance(result, dict):
                if any(x.get('objectId') == 'RiffApiLocalProfile' for x in result.get('results', [])):
                    flags.append('fallback_profile')
        record = {
            'time': datetime.now(timezone.utc).isoformat(), 'session': session,
            'request_id': g.trace_id, 'method': request.method, 'path': request.path,
            'route': str(request.url_rule) if request.url_rule else None,
            'query': redact(parsed), 'request_headers': redact(dict(request.headers)),
            'request_body': body_info(request.get_data(cache=True), request.content_type or ''),
            'status': response.status_code, 'duration_ms': round((time.monotonic()-g.trace_start)*1000, 2),
            'response_headers': redact(dict(response.headers)),
            'response_body': response_body,
            'flags': sorted(set(flags)), 'exception': g.trace_error,
            'backend': getattr(g, 'backend', 'local'),
            'backend_reason': getattr(g, 'backend_reason', 'local_only'),
        }
        try:
            write(record)
        except OSError:
            app.logger.exception('Could not write trace')
        print(f'{request.method} {request.path} -> {response.status_code} [{", ".join(record["flags"])}]', flush=True)
        return response

    print(f'Detailed trace: {logfile}', flush=True)
    return logfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server-dir', type=Path, default=Path(__file__).resolve().parent/'server')
    parser.add_argument('--report', type=Path, help='Summarize a JSONL session instead of starting server')
    parser.add_argument('--game-dir', type=Path, default=Path.home()/'.local/share/Steam/steamapps/common/Riff Racer')
    parser.add_argument('--config', type=Path, help='Override the server.ini in the game directory')
    parser.add_argument('--relay', action='store_true', help='Legacy relay mode; do not use with client INI DLL patch')
    args = parser.parse_args()
    if args.report:
        counts, routes = Counter(), Counter()
        for line in args.report.read_text().splitlines():
            row = json.loads(line)
            counts.update(row['flags'])
            routes.update([f'{row["method"]} {row["route"] or row["path"]} -> {row["status"]}'])
        print('Review candidates (empty results alone do not prove missing functionality):')
        for flag, count in counts.most_common():
            print(f'  {count:5} {flag}')
        print('Routes:')
        for route, count in routes.most_common():
            print(f'  {count:5} {route}')
        return
    spec = importlib.util.spec_from_file_location('riffserver', args.server_dir/'riffapi-local.py')
    server = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(server)
    server.ensure_dirs()
    instrument(server, args.server_dir/'data/traces')
    if args.relay:
        from server_routing import configure, load_config
        config_path = args.config or args.game_dir/'server.ini'
        if not config_path.is_file():
            parser.error(f'Client configuration not found: {config_path}')
        print(f'Legacy relay configuration: {config_path}', flush=True)
        configure(server.app, load_config(config_path))
    else:
        print('Local backend only; endpoint selection is handled by the game DLL.', flush=True)
    # Disable access-log query strings (login requests contain credentials).
    import logging
    logging.getLogger('werkzeug').setLevel(logging.ERROR)
    # Windows uses a creation-time-checked process record for the native Stop
    # button. Linux verifies the exact installation's /proc command line.
    if __import__('os').name == 'nt':
        import ctypes
        from ctypes import wintypes
        kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel.GetCurrentProcess.restype = wintypes.HANDLE
        kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
        created, exited, system, user = (wintypes.FILETIME() for _ in range(4))
        if kernel.GetProcessTimes(kernel.GetCurrentProcess(), ctypes.byref(created), ctypes.byref(exited), ctypes.byref(system), ctypes.byref(user)):
            creation = (created.dwHighDateTime << 32) | created.dwLowDateTime
            record = args.server_dir.parent/'server-process.ini'
            record.write_text('[process]\npid='+str(__import__('os').getpid())+'\ncreation='+str(creation)+'\n', encoding='ascii')
    server.app.run(host='127.0.0.1', port=5144, debug=False, threaded=True)


if __name__ == '__main__':
    main()
