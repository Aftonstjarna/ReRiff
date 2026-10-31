"""Serve the existing Flask app with one threaded WSGI process in Docker."""
import importlib.util
from pathlib import Path
import signal

from waitress import create_server


def main():
    source = Path(__file__).resolve().parent.parent/'riffapi-local.py'
    spec = importlib.util.spec_from_file_location('riffapi_container', source)
    backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(backend)
    backend.ensure_dirs()
    # One process preserves the backend's process-local storage lock.
    server = create_server(backend.app, host='0.0.0.0', port=5144, threads=8)

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
