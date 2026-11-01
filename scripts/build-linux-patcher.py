"""Build the offline Linux patcher with its own runtime and license notices."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dotnet', default=shutil.which('dotnet') or 'dotnet')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
output = root/'build/linux-patcher'
subprocess.run([args.dotnet, 'publish', str(root/'Patcher/Patcher.csproj'),
    '-c', 'Release', '-r', 'linux-x64', '--self-contained', 'true',
    '-p:PublishSingleFile=true', '-p:IncludeNativeLibrariesForSelfExtract=true',
    '-p:InvariantGlobalization=true', '-o', str(output)], check=True)
assets = json.loads((root/'Patcher/obj/project.assets.json').read_text())
dependencies = assets['project']['frameworks']['net8.0']['downloadDependencies']
runtime = next(d for d in dependencies if d['name'].lower() == 'microsoft.netcore.app.runtime.linux-x64')
version = runtime['version'].lstrip('[').split(',')[0]
package = next(Path(folder)/runtime['name'].lower()/version
    for folder in assets['packageFolders']
    if (Path(folder)/runtime['name'].lower()/version/'LICENSE.TXT').is_file())
for name in ('LICENSE.TXT', 'THIRD-PARTY-NOTICES.TXT'):
    shutil.copy2(package/name, output/name)
