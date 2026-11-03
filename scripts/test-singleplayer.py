#!/usr/bin/env python3
"""Build and test singleplayer with Mono; original game SDK files stay local."""
import argparse
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir', type=Path, help='Optional installed game folder for original Parse SDK tests')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
output = root/'build/singleplayer-tests'
output.mkdir(parents=True, exist_ok=True)


def run(command):
    subprocess.run(command, cwd=root, check=True, timeout=60)


run(['mcs', '-sdk:4', '-target:library', '-keyfile:Managed/ReRiff.snk',
     '-r:System.Web.Extensions', '-out:'+str(output/'ReRiff.ClientConfig.dll'),
     'ClientConfig/ClientConfig.cs', 'ClientConfig/Singleplayer.cs', 'ClientConfig/NativeReelBridge.cs'])
run(['mcs', '-r:'+str(output/'ReRiff.ClientConfig.dll'),
     '-out:'+str(output/'RoutingSmoke.exe'), 'ClientConfig/Tests/RoutingSmoke.cs'])
run(['mono', str(output/'RoutingSmoke.exe')])
run(['mcs', '-r:'+str(output/'ReRiff.ClientConfig.dll'),
     '-out:'+str(output/'ServerIdentitySmoke.exe'), 'ClientConfig/Tests/ServerIdentitySmoke.cs'])
run(['mono', str(output/'ServerIdentitySmoke.exe')])
if args.game_dir:
    for name in ('Parse.dll', 'Parse.NetFx45.dll'):
        shutil.copy2(args.game_dir/name, output/name)
    # Installed transports can also contain the independent News header hook.
    headers = args.game_dir/'ReRiff.NewsHeaders.dll'
    if headers.is_file():
        shutil.copy2(headers, output/headers.name)
    facades = Path('/usr/lib/mono/4.5/Facades')
    run(['mcs', '-lib:'+str(facades), '-r:System.Runtime', '-r:System.ObjectModel',
         '-r:System.Threading.Tasks', '-r:System.IO',
         *['-r:'+str(output/name) for name in ('ReRiff.ClientConfig.dll', 'Parse.dll', 'Parse.NetFx45.dll')],
         '-out:'+str(output/'SingleplayerSmoke.exe'), 'ClientConfig/Tests/SingleplayerSmoke.cs'])
    run(['mono', str(output/'SingleplayerSmoke.exe')])
