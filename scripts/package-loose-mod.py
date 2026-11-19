#!/usr/bin/env python3
"""Package a native asset tree as one mod ZIP, without nested VFS archives."""
import argparse
from pathlib import Path
import zipfile


def package(source: Path, output: Path, name: str):
    if not name or name.startswith('.') or any(c in name for c in '/\\:') or name in ('.', '..'):
        raise ValueError('Use a single non-hidden mod folder name')
    if source.is_symlink() or not source.is_dir():
        raise ValueError('Asset source must be a real directory')
    source = source.resolve()
    if output.resolve().is_relative_to(source):
        raise ValueError('Write the distribution outside the source tree')
    files = []
    for path in sorted(source.rglob('*')):
        if path.is_symlink():
            raise ValueError('Asset symlinks are not supported')
        relative = path.relative_to(source).as_posix()
        if (not relative.isascii() or relative != relative.lower()
                or any(ord(c) < 32 or c in '\\:' for c in relative)):
            raise ValueError('Native asset paths must use lowercase ASCII')
        if path.is_file():
            if path.suffix.lower() == '.zip' or path.name.startswith(('.reriff-cache-', '.reriff-merged-')):
                raise ValueError('Pass the loose asset tree, without generated archives or caches')
            files.append((path, relative))
    if not 0 < len(files) < 0x3fff:
        raise ValueError('Native volume requires 1–16382 files')
    # Exclusive create prevents silently overwriting an existing distribution.
    try:
        with output.open('xb') as stream, zipfile.ZipFile(stream, 'w', compression=zipfile.ZIP_DEFLATED, allowZip64=False) as bundle:
            for path, relative in files:
                bundle.write(path, f'{name}/assets/{relative}')
    except FileExistsError:
        raise
    except Exception:
        output.unlink(missing_ok=True)
        raise


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--name', required=True)
    args = parser.parse_args()
    package(args.source, args.output, args.name)
