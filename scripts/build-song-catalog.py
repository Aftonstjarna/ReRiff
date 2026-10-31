"""Build the native server identity engine for Linux and Windows Python."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--zig', required=True)
args = parser.parse_args()
output = root/'build/server-sync'
output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ)
env.setdefault('ZIG_GLOBAL_CACHE_DIR', str(Path(tempfile.gettempdir())/'reriff-zig-global'))
env.setdefault('ZIG_LOCAL_CACHE_DIR', str(Path(tempfile.gettempdir())/'reriff-zig-local'))
for target, name in [('x86_64-linux-gnu.2.28', 'libreriff-catalog.so'), ('x86_64-windows-gnu', 'reriff-catalog.dll')]:
    subprocess.run([args.zig, 'c++', '-target', target, '-std=c++17', '-O2', '-shared',
                    '-fPIC', str(root/'ServerSync/catalog.cpp'), '-o', str(output/name),
                    '-static-libstdc++'], check=True, env=env)
