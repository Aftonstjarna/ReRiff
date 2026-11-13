"""Optional bundled backend entry point for Linux AppImage and Windows Python."""
import os
from pathlib import Path
import runpy
import sys

ROOT = Path(__file__).resolve().parent


def main():
    sys.path.insert(0, str(ROOT/'python-libs'))
    sys.path.insert(0, str(ROOT))
    args = sys.argv[1:]
    if not args:
        raise SystemExit('Use --server-control ACTION or --serve.')
    mode, *args = args
    if mode == '--server-control':
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
        if os.name != 'nt':
            assert ssl.create_default_context().get_ca_certs()
        if (ROOT/'server').is_dir():
            import flask
            import waitress
            if os.name == 'nt':
                ctypes.CDLL(str(ROOT/'server/reriff-catalog.dll')).reriff_catalog
                ctypes.CDLL(str(ROOT/'server/reriff-news.dll')).reriff_news
        print('ReRiff bundled Python runtime OK:', sys.version.split()[0])
    else:
        raise SystemExit('Unknown runtime mode: '+mode)


if __name__ == '__main__':
    main()
