#!/usr/bin/env python3
"""Offline ReRiff installer for Linux/Proton and Windows. Close the game first."""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import sys
import tempfile

from shared_patch import apply_patch, digest


def auto_game():
    bases = [Path.home()/'.local/share/Steam', Path.home()/'.steam/steam']
    if os.name == 'nt':
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam') as key:
                bases.insert(0, Path(winreg.QueryValueEx(key, 'SteamPath')[0]))
        except OSError:
            pass
        bases.append(Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'))/'Steam')
    libraries = list(bases)
    for base in bases:
        vdf = base/'steamapps/libraryfolders.vdf'
        if vdf.exists():
            libraries.extend(Path(p.replace('\\\\', '\\')) for p in re.findall(r'"path"\s*"([^"]+)"', vdf.read_text(errors='replace')))
    for library in libraries:
        game = library/'steamapps/common/Riff Racer'
        if (game/'Game.exe').is_file():
            return game
    return None


def backup_base(game):
    return game/'bak'


def replace(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.reriff-', delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(data)
    try:
        if path.exists():
            temporary.chmod(path.stat().st_mode & 0o777)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def install(bundle, game, backups, platform, dry_run=False, restore=False):
    if not (game/'Game.exe').is_file():
        raise ValueError(f'Game.exe was not found in {game}')
    manifest = json.loads((bundle/'manifest.json').read_text())
    for name, expected in manifest['files'].items():
        relative = Path(name)
        if relative.is_absolute() or '..' in relative.parts or ':' in name:
            raise ValueError('Unsafe package path.')
        if digest((bundle/relative).read_bytes()) != expected:
            raise ValueError('Package integrity check failed: '+name)
    state_path = game/'localserver/install-state.json'
    state = json.loads(state_path.read_text()) if state_path.exists() else {}
    if manifest.get('update_only'):
        if restore:
            raise ValueError('This update does not modify game DLLs. Use the original full installer to restore them.')
        for name, expected in manifest['required_existing'].items():
            if not (game/name).is_file() or digest((game/name).read_bytes()) != expected:
                raise ValueError('This is an update-only package. A supported existing ReRiff installation is required: '+name)
        if not (game/'ReRiff.ClientConfig.dll').is_file():
            raise ValueError('This update requires an existing ReRiff client helper.')
    previous_backups = Path(state['backup_dir']) if state.get('backup_dir') else None
    backups = backups or backup_base(game)
    originals, plan = {}, {}
    for name, patch_info in manifest['patches'].items():
        current = (game/name).read_bytes()
        saved = backups/'originals'/name
        candidate = current
        if name == 'ParseOctane.dll':
            candidate = candidate.replace(b'http://127.000000000.0.1:5144', b'https://parseapi.back4app.com')
        if digest(candidate) != patch_info['original']:
            legacy = Path(__file__).resolve()
            candidates = [saved]
            if previous_backups is not None:
                candidates.append(previous_backups/'originals'/name)
            for parent in legacy.parents:
                candidates.append(parent/'backups/client'/name)
            original = next((p.read_bytes() for p in candidates if p.is_file() and digest(p.read_bytes()) == patch_info['original']), None)
            if original is None:
                raise ValueError('Original backup missing or unsupported DLL: '+name)
            allowed = {patch_info['patched'], state.get('patched', {}).get(name)}
            if digest(current) not in allowed:
                raise ValueError('Unknown DLL modification; refusing to overwrite: '+name)
            candidate = original
        originals[name] = candidate
        plan[name] = candidate if restore else apply_patch(candidate, (bundle/patch_info['file']).read_bytes())
    if not restore:
        for path in (bundle/'payload').rglob('*'):
            if path.is_file():
                relative = path.relative_to(bundle/'payload').as_posix()
                plan[relative] = path.read_bytes()
        if platform == 'windows':
            runtime = bundle/'python'
            if not (runtime/'python.exe').is_file():
                raise ValueError('Windows portable Python runtime is missing.')
            for path in runtime.rglob('*'):
                if path.is_file():
                    plan['localserver/windows-python/'+path.relative_to(runtime).as_posix()] = path.read_bytes()
            plan['localserver/windows-python/python313._pth'] = b'python313.zip\n.\n..\n../python-libs\nimport site\n'
            plan['localserver/start-server.cmd'] = b'@echo off\r\n"%~dp0windows-python\\python.exe" "%~dp0trace-server.py" --server-dir "%~dp0server"\r\n'
        else:
            root = game/'localserver'
            interpreter = sys.executable or shutil.which('python3')
            if not interpreter:
                raise ValueError('A native Python 3 interpreter is required for local server startup.')
            script = '#!/bin/sh\nset -eu\nunset PYTHONHOME\nexport PYTHONNOUSERSITE=1\nexport PYTHONPATH='+shlex.quote(str(root/'python-libs'))+'\nexec '+shlex.quote(interpreter)+' '+shlex.quote(str(root/'trace-server.py'))+' --server-dir '+shlex.quote(str(root/'server'))+'\n'
            plan['localserver/start-server.sh'] = script.encode()
        if not (game/'server.ini').exists():
            plan['server.ini'] = b'[server]\nexternal_url =\nfallback_url = http://127.0.0.1:5144\ntimeout_seconds = 5\nshow_server_popup = true\n'
        next_state = {'version': manifest['version'], 'backup_dir': str(backups),
                      'patched': {n: digest(plan[n]) for n in originals} if originals else state.get('patched',manifest.get('required_existing',{}))}
        plan['localserver/install-state.json'] = (json.dumps(next_state, indent=2)+'\n').encode()
    if dry_run:
        print(f'Validated ReRiff {manifest["version"]}: {len(plan)} files; backups: {backups}')
        return plan
    for name, original in originals.items():
        saved = backups/'originals'/name
        if saved.exists() and saved.read_bytes() != original:
            raise ValueError('Original backup conflict: '+str(saved))
        if not saved.exists():
            replace(saved, original)
    previous, changed = {}, []
    try:
        for relative, data in plan.items():
            target = game/relative
            before = target.read_bytes() if target.exists() else None
            if before == data:
                continue
            previous[relative] = before
            if before is not None and relative not in originals:
                version = backups/'versions'/(digest(before)+'-'+target.name)
                if not version.exists():
                    replace(version, before)
            replace(target, data)
            changed.append(relative)
            if target.suffix == '.sh':
                target.chmod(target.stat().st_mode | 0o111)
    except Exception:
        for relative in reversed(changed):
            if previous[relative] is None:
                (game/relative).unlink(missing_ok=True)
            else:
                replace(game/relative, previous[relative])
        raise
    print(('Original DLLs restored.' if restore else 'ReRiff '+manifest['version']+' installed.')+'\nGame: '+str(game)+'\nBackups: '+str(backups)+'\nExisting settings and saved data preserved.')
    return plan


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path)
    parser.add_argument('--backup-dir', type=Path)
    parser.add_argument('--platform', choices=('linux', 'windows'), default='windows' if os.name == 'nt' else 'linux')
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--restore', action='store_true', help='Restore original game DLLs; retain server and saved data')
    args = parser.parse_args()
    game = args.game_dir or auto_game()
    if game is None:
        parser.error('Steam installation not found. Pass --game-dir pointing to the folder containing Game.exe.')
    if args.platform == 'linux' and sys.version_info < (3, 10):
        parser.error('Linux installation requires Python 3.10 or later.')
    try:
        install(Path(__file__).resolve().parent, game.expanduser().resolve(),
                args.backup_dir.expanduser().resolve() if args.backup_dir else None,
                args.platform, args.dry_run, args.restore)
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, 'Installation failed: '+str(error)+'\n')


if __name__ == '__main__':
    main()
