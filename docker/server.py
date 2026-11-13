"""Single-process threaded container backend with persistent storage and traces."""
import importlib.util
import logging
import os
from pathlib import Path
import signal

ROOT = Path(__file__).resolve().parent.parent


def create_app(data_dir=None, public_url=None, lan_url=None, log_dir=None, trace_enabled=True,
               file_origins=None):
    spec = importlib.util.spec_from_file_location('riffserver', ROOT/'server/riffapi-local.py')
    backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(backend)
    data = Path(data_dir or os.environ.get('RERIFF_DATA_DIR', '/data')).resolve()
    backend.DATA_DIR = str(data)
    backend.CLASSES_DIR = str(data/'classes')
    backend.FILES_DIR = str(data/'files')
    logs = Path(log_dir).resolve() if log_dir is not None else data
    if trace_enabled:
        logs.mkdir(parents=True, exist_ok=True)
    backend.LOG_FILE = str(logs/'requests.log') if trace_enabled else os.devnull
    public = (public_url if public_url is not None else os.environ.get('RERIFF_PUBLIC_URL', '')).strip().rstrip('/')
    lan = (lan_url if lan_url is not None else os.environ.get('RERIFF_LAN_URL', '')).strip().rstrip('/')
    backend.FILE_ORIGINS = {
        key: backend._identity_module.origin(value)
        for key, value in (file_origins if file_origins is not None else
                           {'public': public, 'lan': lan}).items() if value
    }
    backend.ensure_dirs()
    spec = importlib.util.spec_from_file_location('reriff_trace', ROOT/'trace-server.py')
    trace = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(trace)
    if trace_enabled:
        trace.instrument(backend, logs/'traces')
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
