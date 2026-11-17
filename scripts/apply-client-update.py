"""Verified-release extraction and installer handoff tool; never downloads code."""
import argparse
import contextlib
import hashlib
import importlib.util
import json
import shutil
from pathlib import Path, PurePosixPath
import sys
import tarfile
import tempfile
import zipfile
import os
import re
import time
try:
    from logging_policy import logging_enabled, TimestampWriter, timestamp_text
except ModuleNotFoundError:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'packaging'))
    from logging_policy import logging_enabled, TimestampWriter, timestamp_text


def safe_name(name):
    path = PurePosixPath(name)
    if path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name:
        raise ValueError('Unsafe archive path')
    return path


def validate_release_package(metadata, archive, expected, platform):
    schema = metadata.get('schema', 1)
    if type(schema) is not int or schema not in (1, 2):
        raise ValueError('Unsupported release metadata schema')
    if schema == 1 and 'packages' not in metadata:
        if archive.name not in metadata.get('artifacts', []):
            raise ValueError('Release package is not listed in metadata')
        return
    packages = metadata.get('packages')
    if schema != 2 or not isinstance(packages, list) or len(packages) > 128:
        raise ValueError('Invalid explicit release package metadata')
    matches = []
    for package in packages:
        if not isinstance(package, dict):
            raise ValueError('Invalid explicit release package metadata')
        for key, limit in [('component',63), ('platform',31), ('architecture',31), ('filename',255)]:
            value = package.get(key)
            if not isinstance(value,str) or not value or len(value.encode('utf8')) > limit:
                raise ValueError('Invalid release package identity')
        if not re.fullmatch(r'[A-Za-z0-9_-][A-Za-z0-9._-]*',package['filename']):
            raise ValueError('Invalid release package filename')
        if type(package.get('size')) is not int or not 0 < package['size'] <= 2**64-1:
            raise ValueError('Invalid release package size')
        if not isinstance(package.get('sha256'),str) or not re.fullmatch(r'[0-9a-fA-F]{64}',package['sha256']):
            raise ValueError('Invalid release package checksum')
        if (package['component'],package['platform'],package['architecture']) == ('client',platform,'x86' if platform=='windows' else 'x86_64'):
            matches.append(package)
    if len(matches)!=1:
        raise ValueError('Expected one client package for this platform and architecture')
    package=matches[0]
    if package['filename']!=archive.name or package['size']!=archive.stat().st_size or package['sha256'].lower()!=expected.lower():
        raise ValueError('Release package metadata mismatch')


def seek_linux_payload(source):
    # Search fixed-size chunks, retaining enough bytes to match a marker that
    # crosses a boundary. A malformed preamble must not allocate the archive.
    marker = b'\n__RERIFF_PAYLOAD__\n'
    tail = b''
    while True:
        chunk = source.read(64 * 1024)
        if not chunk:
            raise ValueError('Invalid Linux release package')
        block = tail + chunk
        position = block.find(marker)
        if position >= 0:
            source.seek(source.tell() - len(block) + position + len(marker))
            return
        tail = block[-(len(marker) - 1):]


def copy_archive_member(source, output, size):
    # Keep memory bounded and reject truncated members before installer code
    # is loaded. Reading through EOF also lets ZipExtFile verify its CRC.
    if size < 0:
        raise ValueError('Invalid archive member size')
    remaining = size
    with output.open('wb') as destination:
        while remaining:
            block = source.read(min(remaining, 1024 * 1024))
            if not block:
                raise ValueError('Truncated archive member')
            destination.write(block)
            remaining -= len(block)
        if source.read(1):
            raise ValueError('Archive member exceeds declared size')


def prepare(archive, expected, work, game, platform, channel=None):
    started = time.monotonic()
    checksum = hashlib.sha256()
    with archive.open('rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            checksum.update(block)
    digest = checksum.hexdigest()
    if digest != expected.lower():
        raise ValueError('Release checksum mismatch')
    print(f'Timing: package verification {(time.monotonic()-started)*1000:.0f} ms', flush=True)
    started = time.monotonic()
    target = work/'extracted'
    target.mkdir()
    total = 0
    if platform == 'windows':
        with zipfile.ZipFile(archive) as bundle:
            for info in bundle.infolist():
                relative = safe_name(info.filename)
                if (info.external_attr >> 16) & 0o170000 == 0o120000:
                    raise ValueError('Archive links are not supported')
                total += info.file_size
                if total > 512*1024*1024:
                    raise ValueError('Release is too large')
                output = target.joinpath(*relative.parts)
                if info.is_dir():
                    output.mkdir(parents=True, exist_ok=True)
                else:
                    output.parent.mkdir(parents=True, exist_ok=True)
                    with bundle.open(info) as source:
                        copy_archive_member(source, output, info.file_size)
    else:
        # The Linux installer is a shell preamble followed by a gzip tar stream.
        with archive.open('rb') as source:
            seek_linux_payload(source)
            with tarfile.open(fileobj=source, mode='r|gz') as bundle:
                for info in bundle:
                    relative = safe_name(info.name)
                    if not (info.isfile() or info.isdir()):
                        raise ValueError('Archive links and special files are not supported')
                    total += info.size
                    if total > 512*1024*1024:
                        raise ValueError('Release is too large')
                    output = target.joinpath(*relative.parts)
                    if info.isdir():
                        output.mkdir(parents=True, exist_ok=True)
                    else:
                        output.parent.mkdir(parents=True, exist_ok=True)
                        with bundle.extractfile(info) as member:
                            copy_archive_member(member, output, info.size)
    manifests = list(target.glob('manifest.json')) + list(target.glob('*/manifest.json'))
    if len(manifests) != 1:
        raise ValueError('Expected one release manifest')
    root = manifests[0].parent
    manifest = json.loads(manifests[0].read_text())
    if manifest.get('platform') != platform or not manifest.get('update_only'):
        raise ValueError('Expected an update-only package for this platform')
    if channel is not None and manifest.get("channel", "stable") != channel:
        raise ValueError("Release channel mismatch")
    metadata_path = work/'latest-release.json'
    if metadata_path.is_file():
        # New launchers use release metadata rather than interpreting the tag.
        # Bind that identity to the verified archive before loading installer code.
        metadata = json.loads(metadata_path.read_text())
        if manifest.get('version') != metadata.get('version'):
            raise ValueError('Release package version mismatch')
        validate_release_package(metadata, archive, expected, platform)
    installer = load_installer(root)
    installer.install(root, game, None, platform, dry_run=True)
    if platform == 'windows':
        # Apply from a private runtime so a future Python upgrade can replace
        # the installation's executable and DLLs without locking itself.
        shutil.copytree(game/'localserver/windows-python', work/'runtime')
    elif (game/'localserver/ReRiff-Runtime-x86_64.AppImage').is_file():
        # Apply from a private copy so replacing the installed runtime is safe.
        for name in ('ReRiff-Runtime-x86_64.AppImage', 'linux-runtime.sh'):
            shutil.copy2(game/'localserver'/name, work/name)
        (work/'ReRiff-Runtime-x86_64.AppImage').chmod(0o755)
    print(f'Timing: extraction and preparation {(time.monotonic()-started)*1000:.0f} ms', flush=True)
    (work/'prepared.json').write_text(json.dumps({'bundle': str(root), 'game': str(game), 'platform': platform}))


def load_installer(root):
    sys.path.insert(0, str(root))
    spec = importlib.util.spec_from_file_location('reriff_update_installer', root/'installer.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    if hasattr(module, 'patch_vanilla'):
        patch_vanilla = module.patch_vanilla
        def compatible_patch(bundle, game, backups, manifest, restore):
            # Older stable installers predate the experimental News hook.
            # Rebuild only this exact known SDK from its verified original,
            # privately; the stable installer's transaction applies the plan.
            sdk = 'Parse.NetFx45.dll'
            known = '4e476a40cb00f65e4f5c3ab761c2bdbfc138dfcd444d618c643d50c6eca0d96b'
            current = game / sdk
            if (manifest.get('channel', 'stable') == 'stable'
                    and current.is_file()
                    and hashlib.sha256(current.read_bytes()).hexdigest() == known
                    and manifest.get('required_existing', {}).get(sdk) != known):
                original = backups / 'originals' / sdk
                expected = manifest.get('vanilla_originals', {}).get(sdk)
                if not original.is_file() or hashlib.sha256(original.read_bytes()).hexdigest() != expected:
                    raise ValueError('A verified original SDK backup is required to restore stable')
                with tempfile.TemporaryDirectory(prefix='reriff-stable-restore-') as temporary:
                    shadow = Path(temporary)
                    for name in manifest['vanilla_originals']:
                        shutil.copy2(original if name == sdk else game/name, shadow/name)
                    return patch_vanilla(bundle, shadow, backups, manifest, restore)
            return patch_vanilla(bundle, game, backups, manifest, restore)
        module.patch_vanilla = compatible_patch
    return module


def apply(work):
    plan = json.loads((work/'prepared.json').read_text())
    root, game = Path(plan['bundle']), Path(plan['game'])
    if plan['platform'] == 'linux':
        # start.exe /unix does not reliably wait for the Unix child. Confirm
        # owned-backend shutdown on the host before replacing server code.
        native = game/'localserver/native/config.json'
        if native.is_file():
            import subprocess
            result = work/'server-stop.ini'
            subprocess.run([str(work/'ReRiff-Runtime-x86_64.AppImage'),
                '--server-control', 'stop', '--root', str(game/'localserver'),
                '--result', str(result)], check=True, timeout=30)
        controller = game/'localserver/server-control.py'
        if controller.is_file():
            spec = importlib.util.spec_from_file_location('reriff_update_stop', controller)
            control = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(control)
            control.stop(game/'localserver')
    load_installer(root).install(root, game, None, plan['platform'])
    (work/'success').write_text('success\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--channel', choices=['stable', 'experimental'])
    parser.add_argument('--prepare', action='store_true')
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--sha256')
    parser.add_argument('--game-dir', type=Path)
    parser.add_argument('--platform', choices=['linux', 'windows'])
    args = parser.parse_args()
    game=args.game_dir
    if game is None:
        try: game=Path(json.loads((args.work/'prepared.json').read_text())['game'])
        except (OSError,ValueError,KeyError): pass
    enabled=game is not None and logging_enabled(game, category="update")
    log_path=game/'logs/update.log' if enabled else Path(os.devnull)
    try:
        log_path.parent.mkdir(parents=True,exist_ok=True)
    except OSError:
        log_path=args.work/'update.log'
    try:
        with log_path.open('a', encoding='utf-8') as log:
            stamped = TimestampWriter(log)
            with contextlib.redirect_stdout(stamped), contextlib.redirect_stderr(stamped):
                try:
                    started = time.monotonic()
                    if args.prepare:
                        prepare(args.archive, args.sha256, args.work, args.game_dir, args.platform, args.channel)
                    else:
                        apply(args.work)
                    print(f"Timing: {'prepare' if args.prepare else 'install'} total {(time.monotonic()-started)*1000:.0f} ms", flush=True)
                finally:
                    stamped.flush()
    except Exception as error:
        with log_path.open('a', encoding='utf-8') as log:
            log.write(timestamp_text(type(error).__name__+': '+str(error)))
        (args.work/'failure').write_text(type(error).__name__+': '+str(error)+'\n', encoding='utf-8')
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
