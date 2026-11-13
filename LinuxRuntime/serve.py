"""Bundled backend, bound to loopback and owned by one game installation."""
import argparse
import importlib.util
import json
import logging
import os
from pathlib import Path
import signal
import socket
import sys
import secrets
import threading
import time
from host_process import file_lock

from local_server import LocalServer, atomic, stamp


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    args = parser.parse_args()
    server = LocalServer(args.root)
    token = os.environ.get('RERIFF_LOCAL_TOKEN', '')
    if len(token) != 64:
        raise ValueError('Start this server through the ReRiff launcher.')
    server.control.mkdir(parents=True, exist_ok=True)
    lock = file_lock(server.control/'server.lock', blocking=False)
    lock.__enter__()
    package = Path(__file__).resolve().parent
    if not (package/'server/riffapi-local.py').is_file():
        raise RuntimeError('This runtime does not include the optional backend.')
    os.environ['RERIFF_DATA_DIR'] = str(server.data)
    # A local-only server never advertises household or external addresses.
    os.environ['RERIFF_PUBLIC_URL'] = server.address
    os.environ.pop('RERIFF_LAN_URL', None)
    os.environ.pop('RERIFF_MIGRATE_FROM_ORIGIN', None)
    sys.path.insert(0, str(package/'server'))
    spec = importlib.util.spec_from_file_location('reriff_backend_entry', package/'docker/server.py')
    backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(backend)
    logging.basicConfig(level=logging.INFO)
    # Reserve the port before opening persistent data. An existing listener
    # must fail startup without touching the backend's files.
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE if os.name == 'nt' else socket.SO_REUSEADDR, 1)
    listener.bind(('127.0.0.1', server.config['port']))
    app = backend.create_app(log_dir=server.logs, trace_enabled=server.logging)
    app.add_url_rule('/reriff-local/status', 'reriff_local_status', lambda: {'token': token})
    from waitress import create_server
    http = create_server(app, sockets=[listener], threads=8)
    if os.name == 'nt':
        from flask import request, abort
        def shutdown():
            time.sleep(.1)
            http.task_dispatcher.shutdown()
            # Wake and close the socket map on Waitress's own I/O thread.
            http.trigger.pull_trigger(lambda: http.asyncore.close_all(map=http._map))
        def request_stop():
            if request.remote_addr != '127.0.0.1' or not secrets.compare_digest(request.headers.get('X-ReRiff-Local-Token', ''), token):
                abort(403)
            threading.Thread(target=shutdown, daemon=True).start()
            return {'stopping': True}
        app.add_url_rule('/reriff-local/stop', 'reriff_local_stop', request_stop, methods=['POST'])
    record = {'pid': os.getpid(), 'start': stamp(os.getpid()), 'owner': server.owner, 'token': token}
    atomic(server.record_path, json.dumps(record)+'\n')
    def stop(signum, frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    try:
        http.run()
    except KeyboardInterrupt:
        pass
    finally:
        http.task_dispatcher.shutdown()
        http.close()
        if server.record() == record:
            server.record_path.unlink(missing_ok=True)
        lock.__exit__(None, None, None)


if __name__ == '__main__':
    main()
