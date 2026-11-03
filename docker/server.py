"""Single-process threaded container backend with persistent storage and traces."""
import importlib.util
import logging
import os
from pathlib import Path
import signal

ROOT = Path(__file__).resolve().parent.parent


def create_app(data_dir=None, public_url=None, lan_url=None):
    spec = importlib.util.spec_from_file_location('riffserver', ROOT/'server/riffapi-local.py')
    backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(backend)
    data = Path(data_dir or os.environ.get('RERIFF_DATA_DIR', '/data')).resolve()
    backend.DATA_DIR = str(data)
    backend.CLASSES_DIR = str(data/'classes')
    backend.FILES_DIR = str(data/'files')
    backend.LOG_FILE = str(data/'requests.log')
    public = (public_url if public_url is not None else os.environ.get('RERIFF_PUBLIC_URL', '')).strip().rstrip('/')
    lan = (lan_url if lan_url is not None else os.environ.get('RERIFF_LAN_URL', '')).strip().rstrip('/')
    backend.FILE_ORIGINS = {
        key: backend._identity_module.origin(value)
        for key, value in {'public': public, 'lan': lan}.items() if value
    }
    backend.ensure_dirs()
    spec = importlib.util.spec_from_file_location('reriff_trace', ROOT/'trace-server.py')
    trace = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(trace)
    trace.instrument(backend, data/'traces')
    return backend.app


def main():
    from waitress import create_server
    logging.basicConfig(level=logging.INFO)
    app = create_app()
    # One process retains the backend's shared storage lock; Waitress serves
    # concurrent game/admin requests using threads, not independent workers.
    server = create_server(app, host='0.0.0.0', port=5144, threads=8)

    def stop(signum, frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    try:
        server.run()
    finally:
        server.task_dispatcher.shutdown()
        server.close()


if __name__ == '__main__':
    main()
