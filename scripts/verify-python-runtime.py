"""Verify Windows bootstrap pins against the official build-time archive."""
import argparse
import hashlib
from pathlib import Path
import re
import zipfile
p=argparse.ArgumentParser();p.add_argument('--archive',type=Path,required=True);args=p.parse_args()
source=(Path(__file__).resolve().parents[1]/'NativeLauncher/python_runtime.h').read_text()
expected=re.search(r'#define PYTHON_RUNTIME_SHA "([a-f0-9]{64})"',source).group(1)
assert hashlib.sha256(args.archive.read_bytes()).hexdigest()==expected
with zipfile.ZipFile(args.archive) as archive:
    for name in ('python.exe','python313.dll','python313.zip'):
        assert hashlib.sha256(archive.read(name)).hexdigest() in source,name
print('PASS: consented Windows Python bootstrap matches the pinned official archive and runtime files')
