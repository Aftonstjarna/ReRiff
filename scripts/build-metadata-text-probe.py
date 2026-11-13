#!/usr/bin/env python3
"""Stage the opt-in metadata bridge patch and its offline .NET smoke test.

Run MetadataTextSmoke.exe --disabled, then MetadataTextSmoke.exe, under the
game's .NET Framework runtime. No server requests are made by this test.
"""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dotnet', default='dotnet')
parser.add_argument('--framework-dir', type=Path, required=True)
parser.add_argument('--runtime-facade', type=Path, required=True)
parser.add_argument('--patcher', type=Path, required=True)
parser.add_argument('--client-dir', type=Path, required=True)
parser.add_argument('--output', type=Path, default=ROOT/'build/metadata-utf8-probe')
args = parser.parse_args()
out = args.output.resolve()
if out == args.client_dir.resolve():
    parser.error('Use a separate probe directory; never build over the installed client.')
out.mkdir(parents=True, exist_ok=True)
version, location = subprocess.check_output([args.dotnet, '--list-sdks'], text=True).splitlines()[-1].split(' ', 1)
csc = Path(location.strip('[]'))/version/'Roslyn/bincore/csc.dll'
refs = ['/reference:'+str(args.framework_dir/name) for name in (
    'mscorlib.dll', 'System.dll', 'System.Core.dll', 'System.Web.Extensions.dll')]
helper = out/'ReRiff.NewsHeaders.dll'
subprocess.run([args.dotnet, str(csc), '/nologo', '/target:library', '/nostdlib+', '/optimize+',
                '/keyfile:'+str(ROOT/'Managed/ReRiff.snk'), '/out:'+str(helper), *refs,
                *map(str, (ROOT/'NewsHeaders'/name for name in (
                    'Headers.cs', 'ClientNews.cs', 'MetadataTrace.cs', 'MetadataText.cs')))], check=True)
original = out/'ParseOctane.installed.dll'
shutil.copy2(args.client_dir/'ParseOctane.dll', original)
subprocess.run([args.dotnet, str(args.patcher), '--metadata-utf8', str(original), str(helper),
                str(out/'ParseOctane.dll')], check=True)
for name in ('Parse.dll', 'Parse.NetFx45.dll', 'ReRiff.ClientConfig.dll'):
    shutil.copy2(args.client_dir/name, out/name)
subprocess.run([args.dotnet, str(csc), '/nologo', '/target:exe', '/platform:x86', '/unsafe',
                '/nostdlib+', '/optimize+', '/out:'+str(out/'MetadataTextSmoke.exe'), *refs,
                '/reference:'+str(args.runtime_facade), '/reference:'+str(helper),
                '/reference:'+str(out/'Parse.dll'), '/reference:'+str(out/'Parse.NetFx45.dll'),
                str(ROOT/'Managed/MetadataTextSmoke.cs')], check=True)
print('Staged metadata bridge and offline smoke test in', out)
