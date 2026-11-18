#!/usr/bin/env python3
"""Exercise original Windows VFS lookup/read in an isolated Wine prefix.

Requires the user's original depot files, Zig, and a disposable Wine prefix
with the game's VC2013 runtime. Never uses the installed game or saves.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'packaging'))
from native_archive import build_volume


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('zig', 'wine', 'prefix', 'reference'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    game = args.reference.resolve() / 'Game.exe'
    expected = '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
    if hashlib.sha256(game.read_bytes()).hexdigest() != expected:
        raise SystemExit('Original executable hash mismatch')
    env = dict(os.environ, WINEPREFIX=str(args.prefix.resolve()), WINEDEBUG='-all')
    (ROOT / 'build').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='mod-override-', dir=ROOT / 'build') as temp:
        root = Path(temp)
        env.update(ZIG_GLOBAL_CACHE_DIR=str(root / 'zig-global'),
                   ZIG_LOCAL_CACHE_DIR=str(root / 'zig-local'))
        exe = root / 'test.exe'
        subprocess.run([str(args.zig.resolve()), 'cc', '-target', 'x86-windows-gnu',
                        '-O2', '-Wall', '-Wextra', '-Werror',
                        str(ROOT / 'ExperimentalMenu/test_mods_vfs.c'), '-o', str(exe),
                        '-Wl,--image-base,0x10000000', '-luser32', '-lgdi32',
                        '-ladvapi32', '-static'], env=env, check=True, timeout=180)
        path = 'graphics/vehicles/decals/alf_01/batchtextures.brres'
        fixtures = [
            ('baseline', {path: 'stock', 'stock.txt': 'fallback'}, root / 'baseline'),
            ('first', {path: 'first mod'}, root / 'mods/a-livery'),
            ('second', {path: 'second mod', 'second.txt': 'second only'}, root / 'mods/z-livery'),
        ]
        for name, files, output in fixtures:
            source = root / 'sources' / name
            for member, text in files.items():
                file = source / member
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(text)
            pair = build_volume(source, output, 'vfs1')
            if output.parent.name == 'mods':
                for file in pair:
                    file.rename(file.with_name('vfs' + file.suffix))
        with zipfile.ZipFile(args.reference / 'vfs.zip') as archive:
            original = archive.read('graphics/vehicles/decals/alf_01/batchTextures.brres')
        if hashlib.sha256(original).hexdigest() != '7805bdcc802a85f5d22e47053a08a50ccc37b158a76312a82d83cded684d26f0':
            raise SystemExit('Original livery hash mismatch')
        (root / 'original-livery.bin').write_bytes(original)
        def windows(path):
            return 'Z:' + str(path.resolve()).replace('/', '\\')
        for mode in ('late', 'startup', 'real-late', 'real-startup'):
            print('Native override fixture:', mode, flush=True)
            subprocess.run([str(args.wine.resolve()), str(exe), windows(game),
                            windows(root / 'mods') + '\\', '2', mode],
                           env=env, check=True, timeout=60)
        print('Native mount-only regression', flush=True)
        subprocess.run([str(args.wine.resolve()), str(exe), windows(game),
                        windows(root / 'mods') + '\\', '2'],
                       env=env, check=True, timeout=60)


if __name__ == '__main__':
    main()
