"""Run existing HTTP suites against disposable storage and an allocated local port."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
from werkzeug.serving import make_server

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='riffapi-regression-') as storage:
    os.environ['RIFFAPI_DATA_DIR'] = storage
    spec = importlib.util.spec_from_file_location('regression_server', ROOT/'riffapi-local.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.ensure_dirs()
    server = make_server('127.0.0.1', 0, module.app, threaded=True)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        env = dict(os.environ, RIFFAPI_TEST_URL=f'http://127.0.0.1:{server.server_port}')
        for name in ('test_api.py', 'test_auth.py', 'test_ops.py'):
            subprocess.run([sys.executable, str(ROOT/'docs/tests'/name)], env=env, check=True)
    finally:
        server.shutdown()
        server.server_close()
        worker.join()
