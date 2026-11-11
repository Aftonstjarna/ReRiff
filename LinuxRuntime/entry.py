"""Bundled Linux Python entry point; no user site packages or system Python."""
import os
from pathlib import Path
import runpy
import sys

ROOT = Path(__file__).resolve().parent


def main():
    sys.path.insert(0, str(ROOT))
    args = sys.argv[1:]
    if not args:
        raise SystemExit('Use --python SCRIPT, --server-control ACTION, or --serve.')
    mode, *args = args
    if mode == '--python':
        if not args:
            raise SystemExit('A Python script is required.')
        script = Path(args[0]).resolve()
        sys.argv = [str(script), *args[1:]]
        sys.path.insert(0, str(script.parent))
        runpy.run_path(str(script), run_name='__main__')
    elif mode == '--server-control':
        sys.argv = [str(ROOT/'local_server.py'), *args]
        runpy.run_path(sys.argv[0], run_name='__main__')
    elif mode == '--serve':
        sys.argv = [str(ROOT/'serve.py'), *args]
        runpy.run_path(sys.argv[0], run_name='__main__')
    elif mode == '--self-test':
        import ctypes
        import hashlib
        import ssl
        import sqlite3
        import zlib
        assert hashlib.sha256(b'ReRiff').hexdigest()
        assert ssl.create_default_context().get_ca_certs()
        if (ROOT/'server').is_dir():
            import flask
            import waitress
        print('ReRiff bundled Python runtime OK:', sys.version.split()[0])
    else:
        raise SystemExit('Unknown runtime mode: '+mode)


if __name__ == '__main__':
    main()
