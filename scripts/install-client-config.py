#!/usr/bin/env python3
"""Install or restore the built client-side INI patch without editing server.ini."""
import argparse
import hashlib
from pathlib import Path
import os

HASHES = {
    'ParseOctane.dll': 'd661606db8658adb54d5be18b3e3a3a6f07c8e738f2d78556ece42355f8e62e4',
    'Parse.NetFx45.dll': '4e2f65147faec13ae5333337e3c7cd9e25028f3119fe817e059b1d2eab678522',
}


def replace(path, data):
    temporary = path.with_name(path.name + '.reriff-installing')
    try:
        temporary.write_bytes(data)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, default=Path.home()/'.local/share/Steam/steamapps/common/Riff Racer')
    parser.add_argument('--restore', action='store_true')
    parser.add_argument('--backup-dir', type=Path, default=Path(__file__).resolve().parent.parent/'backups/client')
    args = parser.parse_args()
    game = args.game_dir.expanduser().resolve()
    build = Path(__file__).resolve().parent.parent/'build'
    backups = args.backup_dir.expanduser().resolve()
    if not (game/'Game.exe').is_file():
        parser.error('Game.exe not found')
    if args.restore:
        originals = {}
        for name, digest in HASHES.items():
            backup = backups/name
            legacy = game/(name+'.reriff-client-original')
            data = (backup if backup.exists() else legacy).read_bytes()
            if hashlib.sha256(data).hexdigest() != digest:
                raise ValueError('Invalid original backup: '+name)
            originals[name] = data
        for name, data in originals.items():
            replace(game/name, data)
        print('Original DLLs restored. INI and saved server data retained.')
        return
    files = {name:(build/name).read_bytes() for name in (*HASHES, 'ReRiff.ClientConfig.dll', 'ReRiff.Notify.dll')}
    originals = {}
    old = b'https://parseapi.back4app.com'
    local = b'http://127.000000000.0.1:5144'
    for name, digest in HASHES.items():
        backup = backups/name
        legacy = game/(name+'.reriff-client-original')
        original_path = backup if backup.exists() else legacy if legacy.exists() else game/name
        data = original_path.read_bytes()
        if name == 'ParseOctane.dll' and data.count(local) == 1:
            data = data.replace(local, old, 1)
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('Unsupported game DLL; refusing to patch: '+name)
        # Avoid silently replacing an unrelated mod or an unknown patch.
        current = (game/name).read_bytes()
        known = [data, files[name]]
        if name == 'ParseOctane.dll': known.append(data.replace(old, local, 1))
        if current not in known:
            raise ValueError('Unknown installed modification; refusing to overwrite: '+name)
        originals[name] = data
    for name, data in originals.items():
        backups.mkdir(parents=True, exist_ok=True)
        backup = backups/name
        if not backup.exists():
            with backup.open('xb') as stream: stream.write(data)
    # Helper first, SDK guard second, endpoint patch last.
    for name in ('ReRiff.Notify.dll', 'ReRiff.ClientConfig.dll', 'Parse.NetFx45.dll', 'ParseOctane.dll'):
        if name not in HASHES and (game/name).exists():
            previous = (game/name).read_bytes()
            if previous != files[name]:
                versions = backups.parent/'versions'
                versions.mkdir(parents=True, exist_ok=True)
                saved = versions/(name+'.'+hashlib.sha256(previous).hexdigest()[:16]+'.bak')
                if not saved.exists():
                    with saved.open('xb') as stream: stream.write(previous)
        replace(game/name, files[name])
    for name, data in originals.items():
        legacy = game/(name+'.reriff-client-original')
        if legacy.exists() and legacy.read_bytes() == data:
            legacy.unlink()
    config = game/'server.ini'
    if not config.exists():
        config.write_text('[server]\nexternal_url =\nfallback_url =\ntimeout_seconds = 5\n')
    print('Client INI patch installed. Restart the game. Settings preserved:',config)
    print('Original backups:',backups)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError) as error:
        raise SystemExit(str(error))
