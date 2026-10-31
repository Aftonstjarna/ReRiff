#!/usr/bin/env python3
"""Build offline installers from built ReRiff binaries and verified original DLLs."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import zipfile
import zlib

from shared_patch import make_patch, digest

ROOT = Path(__file__).resolve().parent.parent
PYTHON_SHA = '9e35319bffedab8b36ad44d8f377cb4f1733bcec257a0b6d04d94f01486ddb38'


def copy(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def build(args):
    version = (ROOT/'VERSION').read_text().strip()
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise ValueError('Invalid VERSION')
    if args.update_only:
        tracked=subprocess.check_output(['git','ls-tree','-r','--name-only','HEAD'],cwd=ROOT,text=True).splitlines()
        if any(name.endswith('.rrpatch') for name in tracked):
            raise ValueError('Remove game-derived deltas from the committed source tree before building a public update release.')
    if digest(args.python_zip.read_bytes()) != PYTHON_SHA:
        raise ValueError('Portable Python checksum does not match the Python.org release.')
    output = ROOT/'dist'/('v'+version)
    stage = ROOT/'build/release'
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    common = stage/'common'
    common.mkdir()
    for name in ('installer.py', 'shared_patch.py'):
        copy(ROOT/'packaging'/name, common/name)
    for name in ('README.md', 'README-LINUX.md', 'LICENSE', 'RELEASE-NOTES.md', 'SERVER-CONFIG.md', 'LAUNCHER.md'):
        copy(ROOT/name, common/name)
    patches = {}
    for name in (() if args.update_only else ('ParseOctane.dll', 'Parse.NetFx45.dll')):
        if args.original_dir:
            original = (args.original_dir/name).read_bytes()
            patched = (ROOT/'build'/name).read_bytes()
            encoded = make_patch(original, patched)
        else:
            encoded = (args.patch_dir/(name+'.rrpatch')).read_bytes()
        metadata = json.loads(zlib.decompress(encoded))
        path = common/'patches'/(name+'.rrpatch')
        path.parent.mkdir(exist_ok=True)
        path.write_bytes(encoded)
        patches[name] = {'file': path.relative_to(common).as_posix(), 'original': metadata['original'], 'patched': metadata['patched']}
        print(name, 'delta:',len(encoded),'bytes')
    payload = common/'payload'
    for name in ('ReRiff.ClientConfig.dll','ReRiff.Notify.dll','ReRiff-Launcher.exe','ReRiff-Updater.exe'):
        copy(ROOT/'build'/name, payload/name)
    for name in ('trace-server.py','server_routing.py','steam-launcher.sh','start-server-host.sh','stop-server-host.sh','server-control.py','apply-client-update.py'):
        copy(ROOT/'scripts'/name,payload/'localserver'/name)
    for path in (ROOT/'vendor/riffapi-local-extended').iterdir():
        if path.is_file():
            copy(path,payload/'localserver/server'/path.name)
    copy(ROOT/'LICENSE',payload/'localserver/licenses/ReRiff.txt')
    copy(ROOT/'ServerSync/third_party/LICENSE.MIT',payload/'localserver/licenses/nlohmann-json.txt')
    libs = payload/'localserver/python-libs'
    for requirement in (ROOT/'packaging/requirements.txt').read_text().splitlines():
        name, version_pin = requirement.split('==')
        normalized = name.lower().replace('-', '_')
        wheel = next((p for p in args.wheels_dir.glob('*.whl') if p.name.lower().startswith(normalized+'-'+version_pin+'-')), None)
        if wheel is None:
            raise ValueError('Missing pinned wheel: '+requirement)
        with zipfile.ZipFile(wheel) as archive:
            for item in archive.infolist():
                # MarkupSafe has a supported pure-Python fallback. Do not bundle
                # platform-specific accelerators into the shared library tree.
                if item.is_dir() or item.filename.endswith(('.pyd','.so','.pyc')):
                    continue
                dest = libs/item.filename
                if '..' in Path(item.filename).parts or Path(item.filename).is_absolute():
                    raise ValueError('Unsafe wheel path')
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(archive.read(item))
    output.mkdir(parents=True, exist_ok=True)
    commit = subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    for platform in ('linux','windows'):
        folder = stage/platform
        shutil.copytree(common,folder)
        library = 'reriff-catalog.dll' if platform == 'windows' else 'libreriff-catalog.so'
        copy(ROOT/'build/server-sync'/library, folder/'payload/localserver/server'/library)
        if platform == 'windows':
            copy(ROOT/'build/ReRiff-Setup.exe',folder/'ReRiff-Setup.exe')
            with zipfile.ZipFile(args.python_zip) as archive:
                archive.extractall(folder/'python')
            (folder/'python/python313._pth').write_text('python313.zip\n.\n..\nimport site\n')
        manifest = {'version':version,'commit':commit,'platform':platform,'patches':patches,
                    'update_only':args.update_only,
                    'required_existing':json.loads((ROOT/'packaging/required-client.json').read_text()) if args.update_only else {},
                    'files':{p.relative_to(folder).as_posix():digest(p.read_bytes()) for p in sorted(folder.rglob('*')) if p.is_file()}}
        (folder/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        if platform=='windows':
            with zipfile.ZipFile(output/f'ReRiff-{version}-windows.zip','w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
                for path in sorted(folder.rglob('*')):
                    if path.is_file():
                        archive.write(path, f'ReRiff-{version}-windows/'+path.relative_to(folder).as_posix())
        else:
            archive_bytes = io.BytesIO()
            with tarfile.open(fileobj=archive_bytes,mode='w:gz') as archive:
                for path in sorted(folder.iterdir()):
                    archive.add(path,arcname=path.name,filter=lambda info: None if '__pycache__' in info.name else info)
            shell = '''#!/bin/sh
set -eu
command -v python3 >/dev/null 2>&1 || { echo "Python 3.10+ is required." >&2; exit 1; }
work=$(mktemp -d "${TMPDIR:-/tmp}/reriff-install.XXXXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
line=$(awk '/^__RERIFF_PAYLOAD__$/ { print NR+1; exit }' "$0")
tail -n +"$line" "$0" | tar -xz -C "$work"
python3 "$work/installer.py" --platform linux "$@"
exit $?
__RERIFF_PAYLOAD__
'''
            run = output/f'ReRiff-{version}-linux.run'
            run.write_bytes(shell.encode()+archive_bytes.getvalue());run.chmod(0o755)
    # Source archive excludes original game files, player data, and caches.
    with (output/f'ReRiff-{version}-source.tar.gz').open('wb') as stream:
        subprocess.run(['git','archive','--format=tar.gz',f'--prefix=ReRiff-{version}/','HEAD'],cwd=ROOT,stdout=stream,check=True)
    artifacts = sorted(p for p in output.iterdir() if p.suffix in ('.run','.zip','.gz'))
    (output/'SHA256SUMS').write_text(''.join(digest(p.read_bytes())+'  '+p.name+'\n' for p in artifacts))
    (output/'release.json').write_text(json.dumps({'version':version,'commit':commit,'artifacts':[p.name for p in artifacts]},indent=2)+'\n')
    copy(ROOT/'RELEASE-NOTES.md',output/'RELEASE-NOTES.md')
    copy(ROOT/'README-LINUX.md',output/'README-LINUX.md')
    print('Release artifacts:',output)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    inputs=parser.add_mutually_exclusive_group(required=True)
    inputs.add_argument('--original-dir',type=Path)
    inputs.add_argument('--patch-dir',type=Path,help='Use versioned deltas without accessing original game DLLs')
    inputs.add_argument('--update-only',action='store_true',help='Update only ReRiff components; include no game-derived binary patches')
    parser.add_argument('--wheels-dir',type=Path,required=True)
    parser.add_argument('--python-zip',type=Path,required=True)
    build(parser.parse_args())
