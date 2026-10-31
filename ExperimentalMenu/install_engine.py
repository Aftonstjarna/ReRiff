#!/usr/bin/env python3
"""Install/restore only the experimental notification-entry DLL. Close Game.exe first."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

ROOT = Path(__file__).resolve().parent.parent
ORIGINAL = '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace(source, destination):
    fd, name = tempfile.mkstemp(prefix='.reriff-engine-', dir=destination.parent)
    os.close(fd)
    staging = Path(name)
    try:
        shutil.copy2(source, staging)
        os.replace(staging, destination)
    finally:
        staging.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, required=True)
    parser.add_argument('--restore', action='store_true')
    args = parser.parse_args()
    game = args.game_dir.resolve()
    if digest(game / 'Game.exe') != ORIGINAL:
        raise SystemExit('Unsupported Game.exe; nothing changed.')
    destination = game / 'ReRiff.Notify.dll'
    backup_dir = ROOT / 'backups/experimental-engine-menu'
    backup_dir.mkdir(parents=True, exist_ok=True)
    record_path = backup_dir / (hashlib.sha256(str(game).encode()).hexdigest() + '.json')
    record = json.loads(record_path.read_text()) if record_path.exists() else None
    current = digest(destination)
    if args.restore:
        if not record:
            raise SystemExit('No recorded experimental installation; nothing changed.')
        if current not in record.get('replaceable_sha256',
                                     [record['installed_sha256'], record['original_sha256']]):
            raise SystemExit('DLL changed since installation; refusing to overwrite it.')
        backup = backup_dir / (record['original_sha256'] + '.dll')
        if digest(backup) != record['original_sha256']:
            raise SystemExit('Backup verification failed; nothing changed.')
        replace(backup, destination)
        record_path.unlink()
        print('Restored original notification DLL. Game.exe and settings unchanged.')
        return
    source = ROOT / 'build/experimental-engine-menu/ReRiff.Notify.dll'
    built = digest(source)
    if record:
        if current != record['installed_sha256']:
            raise SystemExit('Existing installation changed; restore/review it before reinstalling.')
        if current == built:
            print('Engine-menu build already installed.')
            return
        original = record['original_sha256']
    else:
        original = current
        backup = backup_dir / (original + '.dll')
        if backup.exists():
            if digest(backup) != original:
                raise SystemExit('Existing backup verification failed; nothing changed.')
        else:
            shutil.copy2(destination, backup)
        if digest(backup) != original:
            raise SystemExit('Backup verification failed; nothing changed.')
    # Record recovery information before replacing the destination.
    record_path.write_text(json.dumps({'game_dir': str(game), 'original_sha256': original,
                                      'installed_sha256': built,
                                      'replaceable_sha256': sorted({original, current, built})}, indent=2) + '\n')
    replace(source, destination)
    if digest(destination) != built:
        raise SystemExit('Installed DLL verification failed; use --restore.')
    print('Engine-menu DLL installed and verified; original backed up. Game.exe and settings unchanged.')


if __name__ == '__main__':
    main()
