"""Privately rebuild supported game hooks and compare their exact release hashes."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--originals',type=Path,required=True);p.add_argument('--dotnet',default='dotnet');p.add_argument('--write',action='store_true');a=p.parse_args()
original=json.loads((ROOT/'packaging/original-client.json').read_text());out=ROOT/'build/cjk-client-validation';out.mkdir(parents=True,exist_ok=True)
for name,expected in original.items():
 assert hashlib.sha256((a.originals/name).read_bytes()).hexdigest()==expected,'Reference differs: '+name
host=ROOT/'build/patcher/Patcher.dll'
def patch(mode,source,helper,target):subprocess.run([a.dotnet,str(host),*mode,str(source),str(ROOT/'build'/helper),str(out/target)],check=True)
patch([],a.originals/'ParseOctane.dll','ReRiff.ClientConfig.dll','ParseOctane.dll')
patch(['--offline-gate'],a.originals/'Parse.NetFx45.dll','ReRiff.ClientConfig.dll','offline.dll')
patch(['--news-header'],out/'offline.dll','ReRiff.NewsHeaders.dll','Parse.NetFx45.dll')
result={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in original}
if a.write:(ROOT/'packaging/required-client.json').write_text(json.dumps(result,indent=2)+'\n')
else:assert result==json.loads((ROOT/'packaging/required-client.json').read_text()),'Client patch bytes differ from manifest'
print(json.dumps(result,indent=2))
