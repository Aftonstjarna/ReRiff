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
import subprocess
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


def patch_vanilla(bundle, game, backups, manifest, restore):
    """Run the source-built patcher on private copies, before changing the game."""
    supported = manifest['vanilla_originals']
    expected = manifest['required_existing']
    originals, plan, needs_patch = {}, {}, []
    legacy = manifest.get("compatible_existing", {})
    upgrades = manifest.get('client_upgrades', {})
    for name, original_hash in supported.items():
        path = game/name
        if not path.is_file():
            raise ValueError('Required game DLL is missing: '+str(path))
        current = path.read_bytes()
        current_hash = digest(current)
        upgrade_outputs = upgrades.get(name, {})
        ready = {expected[name], *upgrade_outputs.values()}
        if current_hash not in ({original_hash, legacy.get(name)} | ready | set(upgrade_outputs)):
            raise ValueError('Unsupported game DLL; refusing to overwrite: '+str(path))
        saved = backups/'originals'/name
        if saved.exists():
            data = saved.read_bytes()
            if digest(data) != original_hash:
                raise ValueError('Original backup hash mismatch; backup retained: '+str(saved))
            originals[name] = data
        elif current_hash == original_hash:
            originals[name] = current
        elif restore:
            raise ValueError('Original backup missing: '+str(saved))
        if restore:
            plan[name] = originals[name]
        elif current_hash not in ready:
            needs_patch.append((name, name in originals, current_hash))
    if needs_patch:
        patcher = bundle/'patcher/ReRiff-Patcher.exe'
        command = [str(patcher)]
        if os.name != 'nt':
            mono = shutil.which('mono')
            if not mono:
                raise ValueError('Installing or updating client DLL hooks on Linux requires Mono. Install Mono and retry; no game files have been changed.')
            command.insert(0, mono)
        with tempfile.TemporaryDirectory(prefix='reriff-patch-') as temporary:
            for name, vanilla, input_hash in needs_patch:
                source, output = Path(temporary)/name, Path(temporary)/('patched-'+name)
                source.write_bytes(originals[name] if vanilla else (game/name).read_bytes())
                def run_patch(mode, source, helper, output):
                    result = subprocess.run(command+mode+[str(source), str(bundle/('payload/'+helper)), str(output)],
                                            capture_output=True, text=True, timeout=120)
                    if result.returncode:
                        raise ValueError('Local DLL patching failed for '+name+': '+result.stdout+result.stderr)
                if not vanilla and input_hash in upgrades.get(name, {}):
                    if name == 'Parse.NetFx45.dll' and input_hash == legacy.get(name):
                        with_news = Path(temporary)/'with-news.dll'
                        run_patch(['--news-header'], source, 'ReRiff.NewsHeaders.dll', with_news)
                        source = with_news
                    # Existing hooks retain their IL; only the references to
                    # ReRiff's own helpers change to the signed identities.
                    result = subprocess.run(command+['--retarget-helpers',str(source),str(bundle/'payload'),str(output)],
                                            capture_output=True,text=True,timeout=120)
                    if result.returncode:
                        raise ValueError('Client helper reference repair failed: '+result.stdout+result.stderr)
                elif name == 'ParseOctane.dll':
                    run_patch([], source, 'ReRiff.ClientConfig.dll', output)
                else:
                    if vanilla:
                        offline = Path(temporary)/'offline.dll'
                        run_patch(['--offline-gate'], source, 'ReRiff.ClientConfig.dll', offline)
                        source = offline
                    run_patch(['--news-header'], source, 'ReRiff.NewsHeaders.dll', output)
                data = output.read_bytes()
                output_hash = expected[name] if vanilla else upgrades.get(name, {}).get(input_hash, expected[name])
                if digest(data) != output_hash:
                    raise ValueError('Local patch output hash mismatch: '+name)
                plan[name] = data
    return originals, plan


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
    local_patching = bool(manifest.get('vanilla_originals'))
    if manifest.get('update_only') and not local_patching:
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
    if local_patching:
        originals, plan = patch_vanilla(bundle, game, backups, manifest, restore)
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
                      'patched': {n: digest(plan[n] if n in plan else (game/n).read_bytes()) for n in manifest['required_existing']} if local_patching else ({n: digest(plan[n]) for n in originals} if originals else state.get('patched',manifest.get('required_existing',{})))}
        plan['localserver/install-state.json'] = (json.dumps(next_state, indent=2)+'\n').encode()
    if dry_run:
        print(f'Validated ReRiff {manifest["version"]}: {len(plan)} files; backups: {backups}')
        return plan
    for name, original in originals.items():
        saved = backups/'originals'/name
        if saved.exists() and digest(saved.read_bytes()) != digest(original):
            raise ValueError('Original backup conflict: '+str(saved))
    for name, original in originals.items():
        saved = backups/'originals'/name
        if not saved.exists():
            saved.parent.mkdir(parents=True, exist_ok=True)
            try:
                with saved.open('xb') as stream:
                    stream.write(original)
            except FileExistsError:
                if digest(saved.read_bytes()) != digest(original):
                    raise ValueError('Original backup conflict: '+str(saved))
        else:
            print('Verified vanilla backup; kept unchanged: '+str(saved))
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
