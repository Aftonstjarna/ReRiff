#!/usr/bin/env python3
"""Build the client helper and patches using a .NET 8 SDK and .NET 4.x references."""
import argparse
from pathlib import Path
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--dotnet', default=shutil.which('dotnet') or 'dotnet')
p.add_argument('--framework-dir', type=Path, required=True, help='Directory containing .NET Framework mscorlib.dll and System.dll')
p.add_argument('--original-dir', type=Path, required=True, help='Unmodified ParseOctane.dll and Parse.NetFx45.dll')
a = p.parse_args()
root = Path(__file__).resolve().parent.parent
build = root/'build'
build.mkdir(exist_ok=True)
sdk = subprocess.check_output([a.dotnet,'--list-sdks'],text=True).splitlines()
version, location = sdk[-1].split(' ',1)
csc = Path(location.strip('[]'))/version/'Roslyn/bincore/csc.dll'
subprocess.run([a.dotnet,str(csc),'/nologo','/target:library','/nostdlib+','/optimize+',
                '/out:'+str(build/'ReRiff.ClientConfig.dll'),
                '/reference:'+str(a.framework_dir/'mscorlib.dll'),'/reference:'+str(a.framework_dir/'System.dll'),
                str(root/'ClientConfig/ClientConfig.cs')],check=True)
subprocess.run([a.dotnet,'build',str(root/'Patcher/Patcher.csproj'),'-c','Release','-o',str(build/'patcher')],check=True)
for name in ('ParseOctane.dll','Parse.NetFx45.dll'):
    command = [a.dotnet,str(build/'patcher/Patcher.dll')]
    if name == 'Parse.NetFx45.dll': command.append('--offline-gate')
    subprocess.run(command+[str(a.original_dir/name),str(build/'ReRiff.ClientConfig.dll'),str(build/name)],check=True)
