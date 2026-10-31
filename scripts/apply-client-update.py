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
import zipfile


def safe_name(name):
    path = PurePosixPath(name)
    if path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name:
        raise ValueError('Unsafe archive path')
    return path


def prepare(archive, expected, work, game, platform):
    if hashlib.sha256(archive.read_bytes()).hexdigest() != expected.lower():
        raise ValueError('Release checksum mismatch')
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
                    output.write_bytes(bundle.read(info))
    else:
        # The Linux installer is a shell preamble followed by a gzip tar stream.
        payload = archive.read_bytes().split(b'\n__RERIFF_PAYLOAD__\n', 1)
        if len(payload) != 2:
            raise ValueError('Invalid Linux release package')
        import io
        with tarfile.open(fileobj=io.BytesIO(payload[1]), mode='r:gz') as bundle:
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
                    output.write_bytes(bundle.extractfile(info).read())
    manifests = list(target.glob('manifest.json')) + list(target.glob('*/manifest.json'))
    if len(manifests) != 1:
        raise ValueError('Expected one release manifest')
    root = manifests[0].parent
    manifest = json.loads(manifests[0].read_text())
    if manifest.get('platform') != platform or not manifest.get('update_only'):
        raise ValueError('Expected an update-only package for this platform')
    installer = load_installer(root)
    installer.install(root, game, None, platform, dry_run=True)
    if platform == 'windows':
        # Apply from a private runtime so a future Python upgrade can replace
        # the installation's executable and DLLs without locking itself.
        shutil.copytree(game/'localserver/windows-python', work/'runtime')
    (work/'prepared.json').write_text(json.dumps({'bundle': str(root), 'game': str(game), 'platform': platform}))


def load_installer(root):
    sys.path.insert(0, str(root))
    spec = importlib.util.spec_from_file_location('reriff_update_installer', root/'installer.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def apply(work):
    plan = json.loads((work/'prepared.json').read_text())
    root, game = Path(plan['bundle']), Path(plan['game'])
    if plan['platform'] == 'linux':
        # start.exe /unix does not reliably wait for the Unix child. Confirm
        # owned-backend shutdown on the host before replacing server code.
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
    parser.add_argument('--prepare', action='store_true')
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--sha256')
    parser.add_argument('--game-dir', type=Path)
    parser.add_argument('--platform', choices=['linux', 'windows'])
    args = parser.parse_args()
    try:
        with (args.work/'update.log').open('a', encoding='utf-8') as log, contextlib.redirect_stdout(log), contextlib.redirect_stderr(log):
            if args.prepare:
                prepare(args.archive, args.sha256, args.work, args.game_dir, args.platform)
            else:
                apply(args.work)
    except Exception as error:
        with (args.work/'update.log').open('a', encoding='utf-8') as log:
            log.write(type(error).__name__+': '+str(error)+'\n')
        (args.work/'failure').write_text(type(error).__name__+'\n')
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
