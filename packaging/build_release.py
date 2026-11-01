#!/usr/bin/env python3
"""Build offline installers from built ReRiff binaries and verified original DLLs."""
import argparse
import hashlib
import io
import os
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import zipfile
import zlib

from shared_patch import make_patch, digest
from release_notes import release_notes

ROOT = Path(__file__).resolve().parent.parent
PYTHON_SHA = '9e35319bffedab8b36ad44d8f377cb4f1733bcec257a0b6d04d94f01486ddb38'


def copy(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def build(args):
    channel = os.environ.get('RERIFF_RELEASE_CHANNEL', 'stable')
    if channel not in ('stable', 'experimental'):
        raise ValueError('Invalid release channel')
    version = (ROOT/'VERSION').read_text().strip()
    notes = release_notes((ROOT/'RELEASE-NOTES.md').read_text(encoding='utf-8'), version)
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:-[1-9]\d*)?', version):
        raise ValueError('Invalid VERSION')
    # Retain the flag understood by existing automatic updaters: packages
    # contain no game-derived deltas. vanilla_originals enables local patching.
    update_only = args.update_only or args.local_patching
    if update_only:
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
    copy(ROOT/'LICENSE', common/'LICENSE')
    (common/'RELEASE-NOTES.md').write_text(notes, encoding='utf-8')
    if args.local_patching:
        for name in ('ReRiff-Patcher.exe', 'ReRiff-Patcher.exe.config', 'dnlib.dll'):
            copy(ROOT/'build/installer-patcher'/name, common/'patcher'/name)
        copy(ROOT/'Patcher/dnlib-LICENSE.txt', common/'patcher/dnlib-LICENSE.txt')
    patches = {}
    for name in (() if update_only else ('ParseOctane.dll', 'Parse.NetFx45.dll')):
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
    copy(ROOT/'scripts/Collect-ReRiffStartup.ps1', payload/'Collect-ReRiffStartup.ps1')
    copy(ROOT/'packaging/server.ini.example', payload/'server.ini.example')
    for name in ('ReRiff.ClientConfig.dll','ReRiff.Notify.dll','ReRiff-Launcher.exe','ReRiff-Updater.exe'):
        copy(ROOT/'build'/name, payload/name)
    copy(ROOT/'build/ReRiff.NewsHeaders.dll', payload/'ReRiff.NewsHeaders.dll')
    copy(ROOT/'VERSION', payload/'localserver/client-version.txt')
    (payload/'localserver/client-channel.ini').write_text('[client]\nchannel='+channel+'\n')
    # Multiplayer backend dependencies belong in the container image. The
    # installed client retains only updater and legacy-server cleanup tools.
    for name in ('steam-launcher.sh', 'stop-server-host.sh', 'server-control.py', 'apply-client-update.py'):
        copy(ROOT/'scripts'/name, payload/'localserver'/name)
    output.mkdir(parents=True, exist_ok=True)
    commit = subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    for platform in ('linux','windows'):
        folder = stage/platform
        shutil.copytree(common,folder)
        if args.local_patching and platform == 'linux':
            copy(ROOT/'build/linux-patcher/Patcher', folder/'patcher/ReRiff-Patcher')
            for name in ('LICENSE.TXT', 'THIRD-PARTY-NOTICES.TXT'):
                copy(ROOT/'build/linux-patcher'/name, folder/'patcher'/('dotnet-'+name))
        if platform == 'linux':
            copy(ROOT/'build/ReRiff-Setup', folder/'ReRiff-Setup')
        if platform == 'windows':
            copy(ROOT/'build/ReRiff-Setup.exe',folder/'ReRiff-Setup.exe')
            with zipfile.ZipFile(args.python_zip) as archive:
                archive.extractall(folder/'python')
            (folder/'python/python313._pth').write_text('python313.zip\n.\n..\nimport site\n')
        manifest = {'singleplayer': True, 'channel':channel,'version':version,'commit':commit,'platform':platform,'patches':patches,
                    'update_only':update_only,
                    'vanilla_originals':json.loads((ROOT/'packaging/original-client.json').read_text()) if args.local_patching else {},
                    'compatible_existing':json.loads((ROOT/'packaging/compatible-client.json').read_text()) if args.local_patching else {},
                    'client_upgrades':json.loads((ROOT/'packaging/client-upgrades.json').read_text()) if args.local_patching else {},
                    'required_existing':json.loads((ROOT/'packaging/required-client.json').read_text()) if update_only else {},
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
# Explicit installer arguments preserve the headless updater interface.
mode=auto
case "${1:-}" in
  --gui) mode=gui; shift ;;
  --cli) mode=cli; shift ;;
esac
if [ "$mode" = gui ] && [ "$#" -ne 0 ]; then
  echo "Use --gui alone, or pass installer options for terminal mode." >&2
  exit 2
fi
if [ "$mode" = gui ] || { [ "$mode" = auto ] && [ "$#" -eq 0 ] && { [ -n "${DISPLAY:-}" ] || [ -n "${WAYLAND_DISPLAY:-}" ]; }; }; then
  "$work/ReRiff-Setup"
else
  python3 "$work/installer.py" --platform linux "$@"
fi
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
    (output/'RELEASE-NOTES.md').write_text(notes, encoding='utf-8')
    # User documentation lives on the main repository README, not per release.
    (output/'README-LINUX.md').unlink(missing_ok=True)
    print('Release artifacts:',output)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    inputs=parser.add_mutually_exclusive_group(required=True)
    inputs.add_argument('--original-dir',type=Path)
    inputs.add_argument('--patch-dir',type=Path,help='Use versioned deltas without accessing original game DLLs')
    inputs.add_argument('--update-only',action='store_true',help='Update only ReRiff components; include no game-derived binary patches')
    inputs.add_argument('--local-patching',action='store_true',help='Include the source-built patcher for vanilla installs, without game DLLs or binary deltas')
    parser.add_argument('--wheels-dir',type=Path,help=argparse.SUPPRESS) # Legacy build invocation; no server wheels are packaged.
    parser.add_argument('--python-zip',type=Path,required=True)
    build(parser.parse_args())
