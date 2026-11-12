"""Run the real Windows setup silently against a private original-game fixture."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--wine',required=True);p.add_argument('--prefix',type=Path,required=True);p.add_argument('--python',type=Path,required=True);p.add_argument('--originals',type=Path,required=True);a=p.parse_args()
bundle=ROOT/'build/release/windows';manifest=json.loads((bundle/'manifest.json').read_text())
env=dict(os.environ,WINEPREFIX=str(a.prefix.resolve()),WINEDEBUG='-all')
with tempfile.TemporaryDirectory(prefix='windows-cjk-setup-',dir=ROOT/'build') as temp:
 game=Path(temp)/'Game ü with spaces';game.mkdir()
 for name in ('Game.exe','ParseOctane.dll','Parse.NetFx45.dll'):shutil.copyfile(a.originals/name,game/name)
 shutil.copytree(a.python,game/'localserver/windows-python')
 command=[a.wine,str(bundle/'ReRiff-Setup.exe'),'--game-dir','Z:'+str(game.resolve()).replace('/','\\'),'--cjk']
 with (Path(temp)/'wine.log').open('wb') as log:
  run=subprocess.run(command,env=env,stdout=log,stderr=log,timeout=240)
 if run.returncode:
  for path in (game/'logs/installer.log',Path(temp)/'wine.log'):
   if path.is_file():print(path.name,path.read_text(errors='replace')[-8000:])
 assert run.returncode==0,run.returncode
 assert json.loads((game/'localserver/languages/cjk.json').read_text())['version']==manifest['version']
 for name,expected in manifest['required_existing'].items():assert hashlib.sha256((game/name).read_bytes()).hexdigest()==expected,name
 assert 'cjk_enabled = true' in (game/'server.ini').read_text()
 print('PASS: real Windows setup, private Python, .NET Framework patching, exact DLL hashes and bundled CJK checkbox option.')
