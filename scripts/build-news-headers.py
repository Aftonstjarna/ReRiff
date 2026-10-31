"""Build the thin version-header adapter and extend an existing patched transport."""
import argparse
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dotnet', default='/tmp/reriff-dotnet/dotnet')
parser.add_argument('--framework-dir', type=Path, required=True)
parser.add_argument('--input', type=Path, default=root/'build/Parse.NetFx45.dll')
args = parser.parse_args()
output = root/'build/news-headers'
output.mkdir(parents=True, exist_ok=True)
sdks = subprocess.check_output([args.dotnet, '--list-sdks'], text=True).splitlines()
version, location = sdks[-1].split(' ', 1)
csc = Path(location.strip('[]'))/version/'Roslyn/bincore/csc.dll'
helper = root/'build/ReRiff.NewsHeaders.dll'
subprocess.run([args.dotnet, str(csc), '/nologo', '/target:library', '/nostdlib+', '/optimize+',
                '/out:'+str(helper), '/reference:'+str(args.framework_dir/'mscorlib.dll'),
                '/reference:'+str(args.framework_dir/'System.dll'), str(root/'NewsHeaders/Headers.cs')], check=True)
subprocess.run([args.dotnet, 'build', str(root/'Patcher/Patcher.csproj'), '-c', 'Release',
                '-o', str(root/'build/patcher')], check=True)
subprocess.run([args.dotnet, str(root/'build/patcher/Patcher.dll'), '--news-header', str(args.input),
                str(helper), str(output/'Parse.NetFx45.dll')], check=True)
