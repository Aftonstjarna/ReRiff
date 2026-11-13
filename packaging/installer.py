#!/usr/bin/env python3
"""Offline ReRiff installer for Linux/Proton and Windows. Close the game first."""
import argparse
import configparser
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import sys
import subprocess
import tempfile
import shutil
import subprocess

from shared_patch import apply_patch, digest
from logging_policy import logging_enabled, logging_config


def auto_game():
    bases = [Path.home()/'.local/share/Steam', Path.home()/'.steam/steam',
             Path.home()/'.steam/root', Path.home()/'.var/app/com.valvesoftware.Steam/.local/share/Steam']
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


def default_music_dir():
    """Keep the default link scoped to music, regardless of desktop settings."""
    return Path.home()/'Music'


def link_music_folder(game, music):
    """Optional Linux convenience; never delete a user's music directory."""
    candidates = [game/'prefix/pfx', game/'prefix']
    if game.parent.name == 'common' and game.parent.parent.name == 'steamapps':
        candidates.append(game.parent.parent/'compatdata/351990/pfx')
    prefix = next((p.resolve() for p in candidates if (p/'drive_c/users/steamuser').is_dir()), None)
    if prefix is None:
        print('Music link deferred: no initialized Proton prefix found. Start the game once, then rerun setup to link music.')
        return
    destination = prefix/'drive_c/users/steamuser/Music'
    music = music.expanduser().resolve()
    if music == destination or destination in music.parents:
        print('Music link skipped: the source must be outside the prefix Music folder.')
        return
    if destination.is_symlink() and destination.resolve() == music:
        print('Music link already correct: '+str(destination)+' -> '+str(music))
        return
    if destination.exists() and not destination.is_symlink():
        if not destination.is_dir() or any(destination.iterdir()):
            print('Music link skipped: existing prefix Music contains files and was preserved: '+str(destination))
            return
    music.mkdir(parents=True, exist_ok=True)
    if destination.is_symlink():
        # Replacing a link changes no contents in its previous target.
        fd, temporary = tempfile.mkstemp(prefix='.reriff-music-', dir=destination.parent)
        os.close(fd)
        temp = Path(temporary)
        try:
            temp.unlink()
            temp.symlink_to(music, target_is_directory=True)
            os.replace(temp, destination)
        finally:
            temp.unlink(missing_ok=True)
    else:
        removed_empty = destination.is_dir()
        if removed_empty:
            destination.rmdir()  # refuses if anything was added since the check
        try:
            destination.symlink_to(music, target_is_directory=True)
        except OSError:
            if removed_empty and not destination.exists():
                destination.mkdir()
            raise
    print('Music linked: '+str(destination)+' -> '+str(music))


def backup_base(game):
    return game/'.bak'


def prune_config_history(backups, latest=None):
    folder = backups/'configs'
    if not folder.is_dir() or folder.is_symlink(): return
    groups = {}
    for path in folder.iterdir():
        match = re.fullmatch(r'([0-9a-f]{64})-(.+)', path.name)
        if path.is_file() and not path.is_symlink() and match and digest(path.read_bytes()) == match[1]:
            groups.setdefault(match[2].lower(), []).append(path)
    for paths in groups.values():
        paths.sort(key=lambda p:(p == latest, p.stat().st_mtime_ns, p.name), reverse=True)
        for path in paths[2:]: path.unlink()


def backup_previous(backups, name, data, development, modified=None):
    suffix = Path(name).suffix.lower()
    group = 'configs' if suffix in ('.ini', '.cfg', '.conf', '.toml', '.config', '.json', '.xml', '.yaml', '.yml') else 'versions' if development else None
    if group:
        saved = backups/group/(digest(data)+'-'+Path(name).name)
        previous_time = saved.stat().st_mtime_ns if saved.exists() else 0
        if not saved.exists(): replace(saved, data)
        if group == 'configs':
            import time
            stamp = time.time_ns() if modified is None else max(modified, previous_time)
            os.utime(saved, ns=(stamp, stamp))
            prune_config_history(backups, saved if modified is None else None)


def migrate_config_history(backups):
    versions = backups/'versions'
    if versions.is_dir() and not versions.is_symlink():
        for path in versions.iterdir():
            match = re.fullmatch(r'([0-9a-f]{64})-(.+\.(?:ini|cfg|conf|toml|config|json|xml|yaml|yml))', path.name, re.I)
            if path.is_file() and not path.is_symlink() and match:
                raw = path.read_bytes()
                if digest(raw) != match[1].lower(): continue
                backup_previous(backups, match[2], raw, False, path.stat().st_mtime_ns)
                path.unlink()
        if not any(versions.iterdir()): versions.rmdir()
    prune_config_history(backups)


def prune_release_history(backups):
    versions = backups/'versions'
    if not versions.is_dir() or versions.is_symlink(): return
    baseline = json.loads((Path(__file__).parent/'vanilla-reference.json').read_text())['files'] if (Path(__file__).parent/'vanilla-reference.json').exists() else {}
    original_hashes = {item['sha256'] for item in baseline.values()} | {
        'd661606db8658adb54d5be18b3e3a3a6f07c8e738f2d78556ece42355f8e62e4',
        '4e2f65147faec13ae5333337e3c7cd9e25028f3119fe817e059b1d2eab678522',
        '62d1beb2cc5536f42952af06e6bbb54f8cbf3f9655bad55092b402d544e7f6ee',
        'f0904bb0198f81bef4c4520c3ad15d6e3ba2dc3a7d69ebbce6d3ebf715908a1d'}
    for path in versions.iterdir():
        match = re.fullmatch(r'([0-9a-f]{64})-(.+)', path.name)
        if path.is_symlink() or not path.is_file() or not match: continue
        raw = path.read_bytes()
        if digest(raw) != match[1]: continue
        if match[1] in original_hashes:
            target = backups/'originals'/path.name
            if not target.exists(): replace(target, raw)
        else:
            backup_previous(backups, match[2], raw, False, path.stat().st_mtime_ns)
        path.unlink()
    if not any(versions.iterdir()): versions.rmdir()


def migrate_backups(game):
    """Move legacy backups under .bak, retaining conflicting copies separately."""
    def merge(old, new):
        if old.is_symlink() or new.is_symlink():
            raise ValueError('Refusing linked backup folder: '+str(old))
        if not new.exists():
            old.rename(new)
            return
        for entry in old.iterdir():
            target = new/entry.name
            if entry.is_symlink() or target.is_symlink():
                raise ValueError('Refusing linked backup entry: '+str(entry))
            if entry.is_dir() and target.is_dir():
                merge(entry, target)
            elif not target.exists():
                entry.rename(target)
            elif entry.is_file() and target.is_file() and digest(entry.read_bytes()) == digest(target.read_bytes()):
                entry.unlink()
            else:
                import uuid
                entry.rename(new/(entry.name+'.legacy-'+uuid.uuid4().hex))
        old.rmdir()
    old = game/'bak'
    if old.is_dir(): merge(old, game/'.bak')


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


def patch_supported_client(bundle, game, backups, manifest, restore, previous_backups=None):
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
        revision = manifest.get('client_patch_revision', 0) >= 2
        rebuilds = set(manifest.get('rebuild_existing', {}).get(name, []))
        if revision: ready = {expected[name]}
        if current_hash not in ({original_hash, legacy.get(name)} | ready | set(upgrade_outputs) | rebuilds):
            raise ValueError('Unsupported game DLL; refusing to overwrite: '+str(path))
        saved = backups/'originals'/name
        if saved.exists():
            data = saved.read_bytes()
            if digest(data) != original_hash:
                raise ValueError('Original backup hash mismatch; backup retained: '+str(saved))
            originals[name] = data
        elif previous_backups is not None and (previous_backups/'originals'/name).is_file():
            recorded = previous_backups/'originals'/name
            data = recorded.read_bytes()
            if digest(data) != original_hash:
                raise ValueError('Recorded original backup hash mismatch; backup retained: '+str(recorded))
            originals[name] = data
        elif current_hash == original_hash:
            originals[name] = current
        elif restore:
            raise ValueError('Original backup missing: '+str(saved))
        if restore:
            plan[name] = originals[name]
        elif current_hash not in ready:
            if revision and name not in originals:
                raise ValueError('Verified original backup required for the CJK client upgrade: '+name)
            needs_patch.append((name, name in originals, current_hash))
    if needs_patch:
        patcher = bundle/'patcher/ReRiff-Patcher.exe'
        command = [str(patcher)]
        if os.name != 'nt':
            native = bundle/'patcher/ReRiff-Patcher'
            if native.is_file():
                native.chmod(native.stat().st_mode | 0o100)
                command = [str(native)]
            else:
                mono = shutil.which('mono')
                if not mono:
                    raise ValueError('The Linux patcher is missing from this package. Download the complete current Linux installer; no game files have been changed.')
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


def unattended_update_context(bundle, game, platform):
    # Older native updaters already leave an explicit auto-resume command in
    # their private work directory. Recognize that exact, prepared invocation
    # so its first upgrade can receive the new notice too.
    if Path(sys.argv[0]).name != 'apply-client-update.py' or '--prepare' in sys.argv:
        return False
    try:
        work = Path(sys.argv[sys.argv.index('--work')+1]).resolve()
        prepared = json.loads((work/'prepared.json').read_text())
        return ((work/'resume-command.txt').is_file()
                and Path(prepared['bundle']).resolve() == bundle.resolve()
                and Path(prepared['game']).resolve() == game.resolve()
                and prepared['platform'] == platform)
    except (ValueError, IndexError, KeyError, TypeError, OSError):
        return False


def install(bundle, game, backups, platform, dry_run=False, restore=False, music_dir=None, music_link=None, cjk=None, logging=None, latin=None):
    if not (game/'Game.exe').is_file():
        raise ValueError(f'Game.exe was not found in {game}')
    manifest = json.loads((bundle/'manifest.json').read_text())
    development = manifest.get('channel', 'experimental' if '-' in manifest.get('version', '') else 'stable') == 'experimental'
    if platform == 'linux' and manifest.get('native_linux'):
        # Upgrade compatibility for already-installed Python-based launchers.
        # Fresh Linux setup and updates invoke this executable directly.
        helper=bundle/'payload/localserver/ReRiff-ClientHost'
        expected=manifest['files'].get('payload/localserver/ReRiff-ClientHost')
        if not expected or digest(helper.read_bytes()) != expected:
            raise ValueError('Native client helper checksum mismatch')
        helper.chmod(helper.stat().st_mode | 0o100)
        command=[str(helper),'--client-host','install','--bundle',str(bundle),'--game-dir',str(game)]
        if backups is not None: command += ['--backup-dir',str(backups)]
        if cjk is not None: command.append("--cjk" if cjk else "--no-cjk")
        if latin is not None: command.append("--latin" if latin else "--no-latin")
        if logging is not None: command.append('--logging' if logging else '--no-logging')
        if dry_run: command.append('--dry-run')
        if restore: command.append('--restore')
        if music_dir is not None: command += ['--music-dir',str(music_dir)]
        if music_link is False: command.append('--no-music-link')
        result=subprocess.run(command,timeout=180)
        if result.returncode:
            raise ValueError('Native Linux installer failed; see its diagnostics above.')
        return {}
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
    if previous_backups is None and (game/'bak').is_dir():
        previous_backups = game/'bak'
    if previous_backups is not None and not previous_backups.exists() and previous_backups.is_relative_to(game/'bak'):
        previous_backups = game/'.bak'/previous_backups.relative_to(game/'bak')
    backups = backups or backup_base(game)
    originals, plan = {}, {}
    if local_patching:
        originals, plan = patch_supported_client(bundle, game, backups, manifest, restore, previous_backups)
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
            if manifest.get('python_runtime') == 'download':
                if not (game/'localserver/windows-python/python.exe').is_file():
                    raise ValueError('Run ReRiff-Setup.exe to install the private Python runtime first.')
            elif not (runtime/'python.exe').is_file():
                raise ValueError('Windows portable Python runtime is missing.')
            for path in runtime.rglob('*'):
                if path.is_file():
                    plan['localserver/windows-python/'+path.relative_to(runtime).as_posix()] = path.read_bytes()
            plan['localserver/windows-python/python313._pth'] = b'python313.zip\n.\n..\n../python-libs\nimport site\n'
            if not manifest.get('singleplayer'):
                plan['localserver/start-server.cmd'] = b'@echo off\r\n"%~dp0windows-python\\python.exe" "%~dp0trace-server.py" --server-dir "%~dp0server"\r\n'
        elif not manifest.get('singleplayer'):
            root = game/'localserver'
            interpreter = sys.executable or shutil.which('python3')
            if not interpreter:
                raise ValueError('A native Python 3 interpreter is required for local server startup.')
            script = '#!/bin/sh\nset -eu\nunset PYTHONHOME\nexport PYTHONNOUSERSITE=1\nexport PYTHONPATH='+shlex.quote(str(root/'python-libs'))+'\nexec '+shlex.quote(interpreter)+' '+shlex.quote(str(root/'trace-server.py'))+' --server-dir '+shlex.quote(str(root/'server'))+'\n'
            plan['localserver/start-server.sh'] = script.encode()
        if not (game/'server.ini').exists():
            plan['server.ini'] = b'[server]\n; External server example: https://riffapi.hobune.stream/\n; To use it, set external_url = https://riffapi.hobune.stream/\nexternal_url =\nfallback_url =\ntimeout_seconds = 5\nnetworking_enabled = true\nshow_server_popup = true\n\n[launcher]\nskip_launcher = false\nauto_update_on_skip = false\n'
        language_enabled = cjk
        if language_enabled is None:
            settings = configparser.ConfigParser(interpolation=None, strict=False)
            try:
                settings.read(game/'server.ini', encoding='utf-8-sig')
                language_enabled = settings.getboolean('languages', 'cjk_enabled', fallback=settings.getboolean('diagnostics', 'cjk_render_probe', fallback=False))
            except (configparser.Error, ValueError):
                language_enabled = False
        latin_enabled = latin
        if latin_enabled is None:
            settings = configparser.ConfigParser(interpolation=None, strict=False)
            try:
                settings.read(game/'server.ini', encoding='utf-8-sig')
                latin_enabled = settings.getboolean('languages', 'latin_enabled', fallback=False)
            except (configparser.Error, ValueError):
                latin_enabled = False
        if language_enabled or cjk is False or latin_enabled or latin is False:
            component = manifest.get('language_component', {})
            with tempfile.TemporaryDirectory(prefix='reriff-font-plan-') as scratch:
                command = [str(bundle/'patcher/ReRiff-Patcher.exe'), '--language-component', 'stage',
                           '--game-dir', str(game), '--stage', scratch, '--enable', 'true' if language_enabled else 'false']
                if language_enabled:
                    command += ['--asset',str(bundle/component['file']),'--sha256',component['sha256']]
                if latin_enabled or latin is False:
                    command += ['--latin-enable', 'true' if latin_enabled else 'false']
                    if latin_enabled:
                        european = manifest['latin_component']
                        command += ['--latin-asset',str(bundle/european['file']),'--latin-sha256',european['sha256']]
                result = subprocess.run(command, capture_output=True, text=True, timeout=180)
                if result.returncode:
                    raise ValueError('CJK component preflight failed: '+result.stdout+result.stderr)
                for path in Path(scratch).rglob('*'):
                    if path.is_file() and path.name != '.reriff-language-removals.json': plan[path.relative_to(scratch).as_posix()] = path.read_bytes()
                removals = Path(scratch)/'.reriff-language-removals.json'
                if removals.is_file():
                    for name, expected in json.loads(removals.read_text()).items():
                        if not re.fullmatch(r'vfs[1-9a-f][0-9a-f]?\.(dat|zip)', name) or name in plan or (game/name).is_symlink() or digest((game/name).read_bytes()) != expected:
                            raise ValueError('Unsafe or changed obsolete CJK archive: '+name)
                        plan[name] = None
        next_state = {'version': manifest['version'], 'backup_dir': str(backups),
                      'patched': {n: digest(plan[n] if n in plan else (game/n).read_bytes()) for n in manifest['required_existing']} if local_patching else ({n: digest(plan[n]) for n in originals} if originals else state.get('patched',manifest.get('required_existing',{})))}
        if platform == 'linux':
            enabled = music_link if music_link is not None else (True if music_dir is not None else state.get('music_link', True))
            chosen_music = music_dir or (Path(state['music_dir']) if state.get('music_dir') else default_music_dir())
            next_state.update(music_link=enabled, music_dir=str(chosen_music.expanduser().resolve()))
        plan['localserver/install-state.json'] = (json.dumps(next_state, indent=2)+'\n').encode()
        enabled = logging if logging is not None else logging_enabled(game, development)
        plan['server.ini'] = logging_config(plan.get('server.ini', (game/'server.ini').read_bytes() if (game/'server.ini').is_file() else b''), enabled)
        collector = 'localserver/diagnostics/Collect-ReRiffStartup.ps1'
        if enabled and collector in plan: plan['logs/Collect-ReRiffStartup.ps1'] = plan[collector]
        if not enabled:
            plan = {name:data for name,data in plan.items() if not name.startswith('logs/')}
        if unattended_update_context(bundle, game, platform):
            plan['localserver/update-notification.ini'] = ('[update]\nversion='+manifest['version']+'\n').encode('utf-8')
    if restore and state:
        plan['localserver/install-state.json'] = (json.dumps({**state, 'backup_dir':str(backups)}, indent=2)+'\n').encode()
    if dry_run:
        print(f'Validated ReRiff {manifest["version"]}: {len(plan)} files; backups: {backups}')
        return plan
    # Check every existing backup before creating any backup or changing game files.
    for name, original in originals.items():
        saved = backups/'originals'/name
        if saved.exists() and digest(saved.read_bytes()) != digest(original):
            raise ValueError('Original backup conflict: '+str(saved))
    if backups.resolve() == backup_base(game).resolve():
        migrate_backups(game)
    for name, original in originals.items():
        saved = backups/'originals'/name
        if not saved.exists():
            saved.parent.mkdir(parents=True, exist_ok=True)
            # Exclusive creation also prevents a concurrently created backup
            # from being overwritten between validation and writing.
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
                backup_previous(backups, relative, before, development)
            if data is None:
                changed.append(relative)
                target.unlink()
                continue
            replace(target, data)
            changed.append(relative)
            if target.suffix in ('.sh', '.AppImage'):
                target.chmod(target.stat().st_mode | 0o111)
        # Retire the exact previously shipped root collector only after its
        # replacement is installed; preserve a backup and include rollback.
        collector = 'Collect-ReRiffStartup.ps1'
        old = game/collector
        if ('logs/'+collector in plan or 'localserver/diagnostics/'+collector in plan) and old.is_file() and not old.is_symlink():
            before = old.read_bytes()
            if digest(before) == '637d7452d1ba0865e0a0575618b319fde34b7b54b9b5414c5bfbad37983077ef':
                backup_previous(backups, collector, before, development)
                previous[collector] = before
                changed.append(collector)
                old.unlink()
        example = game/'server.ini.example'
        if not restore and 'server.ini.example' not in plan and example.is_file() and not example.is_symlink():
            before = example.read_bytes()
            backup_previous(backups, example.name, before, development)
            previous['server.ini.example'] = before
            changed.append('server.ini.example')
            example.unlink()
        for name in ('ReRiff GitHub.desktop', 'ReRiff GitHub.url'):
            shortcut = game/name
            if not restore and name not in plan and shortcut.is_file() and not shortcut.is_symlink():
                before = shortcut.read_bytes()
                backup_previous(backups, name, before, development)
                previous[name] = before
                changed.append(name)
                shortcut.unlink()
    except Exception:
        for relative in reversed(changed):
            if previous[relative] is None:
                (game/relative).unlink(missing_ok=True)
            else:
                replace(game/relative, previous[relative])
        raise
    if platform == 'linux' and not restore and next_state.get('music_link'):
        try:
            link_music_folder(game, Path(next_state['music_dir']))
        except OSError as error:
            print('Music link was not applied; game installation completed: '+str(error))
    migrate_config_history(backups)
    if not development:
        prune_release_history(backups)
    # Preserve older diagnostics while keeping the game root clear.
    logs=game/'logs'
    try:
        if logging_enabled(game): logs.mkdir(exist_ok=True)
        for name in (('ReRiff-client.log','ReRiff-network.log','ReRiff-engine-menu.log','ReRiff-launcher.log','installer.log') if logging_enabled(game) else ()):
            old=game/name
            if old.is_file():
                target=logs/name
                if target.exists():
                    import time
                    target=logs/('legacy-'+str(time.time_ns())+'-'+name)
                old.rename(target)
    except OSError as error:
        print('Could not relocate legacy diagnostics: '+str(error))
    print(('Original DLLs restored.' if restore else 'ReRiff '+manifest['version']+' installed.')+'\nGame: '+str(game)+'\nBackups: '+str(backups)+'\nExisting settings and saved data preserved.')
    return plan


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--print-music-dir', action='store_true', help='Print the default Linux music folder without installing')
    parser.add_argument('--music-dir', type=Path, help='Linux music folder to link into an initialized Proton prefix')
    parser.add_argument('--no-music-link', dest='music_link', action='store_false', default=None, help='Skip Linux music linking (keeps existing folders/links)')
    parser.add_argument('--print-game-dir', action='store_true', help='Print a detected Steam game folder without installing')
    parser.add_argument('--game-dir', type=Path)
    parser.add_argument('--backup-dir', type=Path)
    parser.add_argument('--platform', choices=('linux', 'windows'), default='windows' if os.name == 'nt' else 'linux')
    parser.add_argument('--latin', dest='latin', action='store_true', default=None, help='Install Latin/European fonts including Polish')
    parser.add_argument('--no-latin', dest='latin', action='store_false', help='Disable Latin/European fonts')
    parser.add_argument('--cjk', dest='cjk', action='store_true', default=None, help='Install and enable CJK fonts')
    parser.add_argument('--no-cjk', dest='cjk', action='store_false', help='Disable CJK fonts and retain installed assets')
    parser.add_argument('--logging', dest='logging', action='store_true', default=None, help='Enable client diagnostic logs')
    parser.add_argument('--no-logging', dest='logging', action='store_false', help='Disable logging; retain existing logs')
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--restore', action='store_true', help='Restore original game DLLs; retain server and saved data')
    args = parser.parse_args()
    if args.print_music_dir:
        print(default_music_dir())
        return
    if args.print_game_dir:
        found = auto_game()
        if found:
            print(found)
        return
    game = args.game_dir or auto_game()
    if game is None:
        parser.error('Steam installation not found. Pass --game-dir pointing to the folder containing Game.exe.')
    if args.platform == 'linux' and sys.version_info < (3, 10):
        parser.error('Linux installation requires Python 3.10 or later.')
    try:
        install(Path(__file__).resolve().parent, game.expanduser().resolve(),
                args.backup_dir.expanduser().resolve() if args.backup_dir else None,
                args.platform, args.dry_run, args.restore, args.music_dir, args.music_link, args.cjk, args.logging, args.latin)
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, 'Installation failed: '+str(error)+'\n')


if __name__ == '__main__':
    main()
