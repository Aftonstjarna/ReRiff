#!/usr/bin/env python3
"""Bundle the optional Windows backend and Flask; never install pip on players' systems."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def build(wheels, download=True):
    wheels.mkdir(parents=True, exist_ok=True)
    if download:
        # An older cached wheel must never override a newly pinned dependency.
        for old in wheels.glob('*.whl'):
            old.unlink()
        subprocess.run([sys.executable, '-m', 'pip', 'download', '--only-binary=:all:', '--platform', 'win_amd64',
                        '--python-version', '313', '--implementation', 'cp', '--abi', 'cp313',
                        '--dest', str(wheels), '-r', str(ROOT/'docker/requirements.txt')], check=True)
    target = ROOT/'build/windows-server/package'
    if target.exists():
        shutil.rmtree(target)
    target.mkdir(parents=True)
    for name in ('entry.py','local_server.py','serve.py','host_process.py','windows_package.py'):
        shutil.copy2(ROOT/'LinuxRuntime'/name, target/name)
    shutil.copytree(ROOT/'vendor/riffapi-local-extended', target/'server', ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
    shutil.copy2(ROOT/'VERSION', target/'server/VERSION')
    (target/'docker').mkdir()
    shutil.copy2(ROOT/'docker/server.py', target/'docker/server.py')
    shutil.copy2(ROOT/'scripts/trace-server.py', target/'trace-server.py')
    shutil.copy2(ROOT/'LICENSE', target/'LICENSE')
    for name in ('reriff-catalog.dll','reriff-news.dll'):
        shutil.copy2(ROOT/'build/server-sync'/name, target/'server'/name)
    shutil.copy2(ROOT/'ServerSync/third_party/LICENSE.MIT', target/'server/JSON-LICENSE.MIT')
    libs = target/'python-libs';libs.mkdir()
    installed = []
    for wheel in sorted(wheels.glob('*.whl')):
        # Dependency wheels are build inputs, not downloaded at server startup.
        with zipfile.ZipFile(wheel) as bundle:
            for info in bundle.infolist():
                relative = Path(info.filename)
                if relative.is_absolute() or '..' in relative.parts or '\\' in info.filename:
                    raise ValueError('Unsafe wheel path')
                output = libs/relative
                if info.is_dir():
                    output.mkdir(parents=True, exist_ok=True)
                else:
                    output.parent.mkdir(parents=True, exist_ok=True);output.write_bytes(bundle.read(info))
        installed.append({'file':wheel.name,'sha256':hashlib.sha256(wheel.read_bytes()).hexdigest()})
    for name in ('flask','waitress','werkzeug','jinja2','markupsafe','click','itsdangerous','blinker','colorama'):
        if not (libs/name).is_dir():
            raise ValueError('Missing server dependency: '+name)
    (target/'build.json').write_text(json.dumps({'version':(ROOT/'VERSION').read_text().strip(), 'python':'3.13-win_amd64', 'wheels':installed},indent=2)+'\n')
    archive = target.parent/'ReRiff-Server-windows.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
        for path in sorted(target.rglob('*')):
            if path.is_file():
                bundle.write(path,path.relative_to(target).as_posix())
    print(archive)
    return archive


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--wheels',type=Path,default=ROOT/'build/windows-server/wheels')
    parser.add_argument('--offline',action='store_true')
    args=parser.parse_args();build(args.wheels,not args.offline)
