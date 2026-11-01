#!/usr/bin/env python3
"""Prepare vanilla or existing ReRiff clients for the engine menu, with rollback."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME_HASH = '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
VANILLA = {'ParseOctane.dll': 'd661606db8658adb54d5be18b3e3a3a6f07c8e738f2d78556ece42355f8e62e4',
           'Parse.NetFx45.dll': '4e2f65147faec13ae5333337e3c7cd9e25028f3119fe817e059b1d2eab678522'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def atomic(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix='.reriff-menu-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
        if path.exists():
            os.chmod(name, path.stat().st_mode)
        os.replace(name, path)
    finally:
        Path(name).unlink(missing_ok=True)


def backup_directory(game, migrate=True):
    destination = game/'bak/experimental-engine-menu'
    legacy = ROOT/'backups/experimental-engine-menu'
    if not migrate:
        return destination
    for name, expected in VANILLA.items():
        current = game/name
        previous = ROOT/'backups/client'/name
        source = current if current.exists() and sha(current.read_bytes()) == expected else previous
        if source.exists() and sha(source.read_bytes()) == expected:
            target = game/'bak/originals'/name
            if target.exists() and sha(target.read_bytes()) != expected:
                raise ValueError('Original DLL backup conflict in bak: '+name)
            if not target.exists():
                atomic(target, source.read_bytes())
    key = sha(str(game).encode())
    for prefix in ('', 'client-', 'news-'):
        old_record = legacy/(prefix+key+'.json')
        if not old_record.exists():
            continue
        raw = old_record.read_bytes()
        record = json.loads(raw)
        references = [(value, '.bin') for value in record.get('before', {}).values() if value]
        if record.get('original_sha256'):
            references.append((record['original_sha256'], '.dll'))
        for expected, extension in references:
            original = legacy/(expected+extension)
            data = original.read_bytes()
            if sha(data) != expected:
                raise ValueError('Legacy backup verification failed; not migrating: '+str(original))
            target = destination/original.name
            if target.exists() and sha(target.read_bytes()) != expected:
                raise ValueError('Backup conflict in bak: '+target.name)
            if not target.exists():
                atomic(target, data)
        target = destination/old_record.name
        if target.exists() and target.read_bytes() != raw:
            target = destination/('legacy-'+old_record.stem+'-'+sha(raw)+'.json')
        atomic(target, raw)
        # Move only this installation's records; shared old binary copies may
        # still be referenced by other trials and are left in place.
        old_record.unlink()
    install_state = game/'localserver/install-state.json'
    if install_state.exists():
        state = json.loads(install_state.read_text())
        old_base = Path(state['backup_dir']) if state.get('backup_dir') else None
        new_base = game/'bak'
        if old_base is not None and old_base.resolve() != new_base.resolve():
            copies = []
            if old_base.is_dir():
                for path in old_base.rglob('*'):
                    if path.is_symlink():
                        raise ValueError('Backup migration requires regular files: '+str(path))
                    if path.is_file():
                        target = new_base/path.relative_to(old_base)
                        data = path.read_bytes()
                        if target.exists() and target.read_bytes() != data:
                            raise ValueError('Conflicting existing backup in bak: '+str(target))
                        copies.append((target, data))
            for target, data in copies:
                if not target.exists():
                    atomic(target, data)
            state['backup_dir'] = str(new_base)
            atomic(install_state, (json.dumps(state, indent=2)+'\n').encode())
    return destination


def transact(game, plan, backup_dir, record_path, preserve_record=False):
    prior = json.loads(record_path.read_text()) if preserve_record and record_path.exists() else None
    if prior:
        for name, expected in prior['installed'].items():
            if not (game/name).exists() or sha((game/name).read_bytes()) != expected:
                raise ValueError('File changed since this trial; refusing upgrade: '+name)
    previous = {}
    for name in plan:
        path = game/name
        previous[name] = path.read_bytes() if path.exists() else None
    backups = dict(prior['before']) if prior else {}
    for name, data in previous.items():
        if name not in backups:
            backups[name] = sha(data) if data is not None else None
        if data is not None:
            saved = backup_dir/(sha(data)+'.bin')
            if saved.exists() and sha(saved.read_bytes()) != sha(data):
                raise ValueError('Backup conflict; nothing installed: '+name)
            if not saved.exists():
                atomic(saved, data)
    changed = []
    old_record = record_path.read_bytes() if record_path.exists() else None
    try:
        # Installation record exists before the first client file changes.
        installed = dict(prior['installed']) if prior else {}
        installed.update({name: sha(data) for name, data in plan.items()})
        atomic(record_path, json.dumps({'game': str(game), 'before': backups,
               'installed': installed}, indent=2).encode())
        for name, data in plan.items():
            atomic(game/name, data)
            changed.append(name)
    except Exception:
        for name in reversed(changed):
            if previous[name] is None:
                (game/name).unlink(missing_ok=True)
            else:
                atomic(game/name, previous[name])
        if old_record is None:
            record_path.unlink(missing_ok=True)
        else:
            atomic(record_path, old_record)
        raise


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game-dir', type=Path, required=True)
    p.add_argument('--dotnet', default='/tmp/reriff-dotnet/dotnet')
    p.add_argument('--framework-dir', type=Path, default=ROOT/'build/song-reference/dotnet452-proton1102-win64/drive_c/windows/Microsoft.NET/Framework/v4.0.30319')
    p.add_argument('--restore', action='store_true')
    p.add_argument('--dry-run', action='store_true')
    p.add_argument('--migrate-backups', action='store_true', help='Move this trial backup ledger into the game bak folder and exit')
    args = p.parse_args()
    game = args.game_dir.resolve()
    if sha((game/'Game.exe').read_bytes()) != GAME_HASH:
        raise ValueError('Unsupported Game.exe; nothing installed.')
    backup_dir = backup_directory(game, migrate=not args.dry_run)
    if args.migrate_backups:
        print('Verified trial backups and records are in', backup_dir); return
    record_path = backup_dir/('client-'+sha(str(game).encode())+'.json')
    if args.restore:
        record = json.loads(record_path.read_text())
        restore = {}
        for name, expected in record['installed'].items():
            path = game/name
            if not path.exists() or sha(path.read_bytes()) != expected:
                raise ValueError('File changed after this trial; refusing to overwrite: '+name)
            old = record['before'][name]
            data = (backup_dir/(old+'.bin')).read_bytes() if old else None
            if data is not None and sha(data) != old:
                raise ValueError('Backup verification failed: '+name)
            restore[name] = data
        if args.dry_run:
            print('Validated restore of', len(restore), 'files.'); return
        for name, data in restore.items():
            if data is None:
                (game/name).unlink()
            else:
                atomic(game/name, data)
        record_path.unlink()
        print('Restored pre-trial client DLLs and metadata; Game.exe and server.ini unchanged.'); return
    if record_path.exists():
        raise ValueError('A setup trial is already recorded; restore it before reinstalling.')
    subprocess.run([args.dotnet, 'build', str(ROOT/'Patcher/Patcher.csproj'), '-c', 'Release',
                    '-o', str(ROOT/'build/patcher')], check=True)
    with tempfile.TemporaryDirectory(prefix='reriff-menu-client-') as directory:
        stage = Path(directory)
        patcher = [args.dotnet, str(ROOT/'build/patcher/Patcher.dll')]
        subprocess.run(patcher+['--inspect-client', str(game), str(stage/'client.json')], check=True)
        state = json.loads((stage/'client.json').read_text())
        for name, patched in [('ParseOctane.dll', state['endpoint']), ('Parse.NetFx45.dll', state['offline'])]:
            if not patched and sha((game/name).read_bytes()) != VANILLA[name]:
                raise ValueError('Unsupported client DLL; nothing installed: '+name)
        sdks = subprocess.check_output([args.dotnet, '--list-sdks'], text=True).splitlines()
        sdk_version, sdk_path = sdks[-1].split(' ', 1)
        csc = Path(sdk_path.strip('[]'))/sdk_version/'Roslyn/bincore/csc.dll'
        for source, output in [('ClientConfig/ClientConfig.cs', 'ReRiff.ClientConfig.dll'),
                               ('NewsHeaders/Headers.cs', 'ReRiff.NewsHeaders.dll')]:
            subprocess.run([args.dotnet, str(csc), '/nologo', '/target:library', '/nostdlib+', '/optimize+',
                            '/out:'+str(stage/output), '/reference:'+str(args.framework_dir/'mscorlib.dll'),
                            '/reference:'+str(args.framework_dir/'System.dll'), str(ROOT/source)], check=True)
        if not state['endpoint']:
            subprocess.run(patcher+[str(game/'ParseOctane.dll'), str(stage/'ReRiff.ClientConfig.dll'),
                                    str(stage/'ParseOctane.dll')], check=True)
        sdk = game/'Parse.NetFx45.dll'
        if not state['offline']:
            subprocess.run(patcher+['--offline-gate', str(sdk), str(stage/'ReRiff.ClientConfig.dll'),
                                    str(stage/'offline.dll')], check=True)
            sdk = stage/'offline.dll'
        if not state['news']:
            subprocess.run(patcher+['--news-header', str(sdk), str(stage/'ReRiff.NewsHeaders.dll'),
                                    str(stage/'Parse.NetFx45.dll')], check=True)
        plan = {name: (stage/name).read_bytes() for name in ('ReRiff.ClientConfig.dll', 'ReRiff.NewsHeaders.dll')}
        for name in VANILLA:
            if (stage/name).exists():
                plan[name] = (stage/name).read_bytes()
        plan['ReRiff.Notify.dll'] = (ROOT/'build/experimental-engine-menu/ReRiff.Notify.dll').read_bytes()
        install_state = game/'localserver/install-state.json'
        version = json.loads(install_state.read_text())['version'] if install_state.exists() else (ROOT/'VERSION').read_text().strip()
        if not re.fullmatch(r'[0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}', version):
            raise ValueError('Cannot determine a valid installed client version; nothing installed.')
        plan['localserver/client-version.txt'] = (version+'\n').encode()
        if args.dry_run:
            print('Validated', 'existing ReRiff' if state['endpoint'] else 'vanilla', 'client;', len(plan), 'files prepared; nothing installed.'); return
        transact(game, plan, backup_dir, record_path)
        print('Engine menu and version reporting installed from', 'existing ReRiff' if state['endpoint'] else 'vanilla',
              'DLLs. Backups verified; Game.exe and server.ini unchanged.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))
