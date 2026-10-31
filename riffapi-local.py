#!/usr/bin/env python3
"""Compatibility entry point for the single maintained ReRiff backend."""
import os
from pathlib import Path
import runpy
import sys

ROOT = Path(__file__).resolve().parent

if __name__ == '__main__':
    os.environ.setdefault('RERIFF_DATA_DIR', os.environ.get('RIFFAPI_DATA_DIR') or str(ROOT/'data'))
    runner = ROOT/'scripts/trace-server.py'
    sys.argv[0] = str(runner)
    sys.argv[1:1] = ['--server-dir', str(ROOT/'vendor/riffapi-local-extended')]
    runpy.run_path(str(runner), run_name='__main__')
