#!/usr/bin/env python3
"""Fetch pinned official SDL headers locally when system headers are absent."""
import hashlib
import io
from pathlib import Path
import subprocess
import tarfile
import urllib.request

root=Path(__file__).resolve().parents[1]
destination=root/'build/native-deps'
headers=destination/'SDL3-3.2.0/include/SDL3/SDL.h'
if headers.exists():
    print('Local SDL3 headers ready.')
    raise SystemExit(0)
try:
    if subprocess.run(['pkg-config','--exists','sdl3'],check=False).returncode==0:
        print('System SDL3 development package ready.')
        raise SystemExit(0)
except FileNotFoundError:
    pass
url='https://github.com/libsdl-org/SDL/releases/download/release-3.2.0/SDL3-3.2.0.tar.gz'
expected='bf308f92c5688b1479faf5cfe24af72f3cd4ce08d0c0670d6ce55bc2ec1e9a5e'
print('Downloading official SDL 3.2.0 headers into build/native-deps...')
with urllib.request.urlopen(url,timeout=60) as response:
    archive=response.read(32*1024*1024+1)
if len(archive)>32*1024*1024 or hashlib.sha256(archive).hexdigest()!=expected:
    raise SystemExit('SDL archive checksum mismatch')
destination.mkdir(parents=True,exist_ok=True)
with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
    members=[m for m in tar.getmembers() if m.name.startswith('SDL3-3.2.0/include/') or m.name=='SDL3-3.2.0/LICENSE.txt']
    tar.extractall(destination,members=members,filter='data')
if not headers.exists():raise SystemExit('SDL headers missing from archive')
print('SDL3 headers verified and ready.')
