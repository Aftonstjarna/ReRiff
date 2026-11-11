"""Bundled backend, bound to loopback and owned by one game installation."""
import argparse
import fcntl
import importlib.util
import json
import logging
import os
from pathlib import Path
import signal
import socket
import sys

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
    lock = (server.control/'server.lock').open('a')
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
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
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(('127.0.0.1', server.config['port']))
    app = backend.create_app(log_dir=server.logs)
    app.add_url_rule('/reriff-local/status', 'reriff_local_status', lambda: {'token': token})
    from waitress import create_server
    http = create_server(app, sockets=[listener], threads=8)
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
        lock.close()


if __name__ == '__main__':
    main()
