#!/usr/bin/env python3
"""Compatibility launcher only; all preparation and rendering run in C++."""
import os
from pathlib import Path
import sys

binary = Path(__file__).resolve().parents[1] / 'build/native-linux/reriff'
if not binary.is_file():
    raise SystemExit('Build the native tools first: make -C NativeLinux all')
os.execv(str(binary), [str(binary), *sys.argv[1:]])
