#!/usr/bin/env python3
"""Fetch checksum-pinned release build tools into an explicit cache directory."""
import argparse
import hashlib
from pathlib import Path
import tarfile
import urllib.request

ASSETS = {
    'zig.tar.xz': ('https://ziglang.org/download/0.14.1/zig-x86_64-linux-0.14.1.tar.xz',
                   '24aeeec8af16c381934a6cd7d95c807a8cb2cf7df9fa40d359aa884195c4716c'),
    'python.zip': ('https://www.python.org/ftp/python/3.13.17/python-3.13.17-embed-amd64.zip',
                   '9e35319bffedab8b36ad44d8f377cb4f1733bcec257a0b6d04d94f01486ddb38'),
}

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--directory',type=Path,required=True)
args=parser.parse_args()
args.directory.mkdir(parents=True,exist_ok=True)
for name,(url,expected) in ASSETS.items():
    file=args.directory/name
    if not file.exists():urllib.request.urlretrieve(url,file)
    if hashlib.sha256(file.read_bytes()).hexdigest()!=expected:
        raise SystemExit('Checksum failed: '+name)
with tarfile.open(args.directory/'zig.tar.xz') as archive:
    archive.extractall(args.directory,filter='data')
print('Verified Zig and portable Python build inputs.')
