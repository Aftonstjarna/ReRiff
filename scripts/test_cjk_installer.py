"""Full private fixture installs: vanilla and the exact local CJK prototype."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--originals',type=Path,required=True);p.add_argument('--prototype',type=Path,required=True);a=p.parse_args()
bundle=ROOT/'build/release/linux';host=bundle/'payload/localserver/ReRiff-ClientHost'
manifest=json.loads((bundle/'manifest.json').read_text())
hash_file=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
with tempfile.TemporaryDirectory(prefix='full-cjk-test-',dir=ROOT/'build') as temp:
 for prototype in (False,True):
  game=Path(temp)/('Prototype ü' if prototype else 'Clean ü');game.mkdir()
  shutil.copy2(a.originals/'Game.exe',game/'Game.exe')
  for name in manifest['required_existing']:
   shutil.copy2((a.prototype if prototype else a.originals)/name,game/name)
   saved=game/'bak/originals'/name;saved.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(a.originals/name,saved)
  (game/'server.ini').write_text('[server]\nexternal_url = https://example.invalid\n'+('[diagnostics]\ncjk_render_probe = 1\n' if prototype else ''))
  (game/'vfs.dat').write_bytes(b'original index fixture');(game/'vfs.zip').write_bytes(b'original archive fixture')
  if prototype:
   for stem in ('vfs2','vfs3'):
    for ext in ('.dat','.zip'):shutil.copy2(a.prototype/(stem+ext),game/(stem+ext))
   shutil.copytree(a.prototype/'fonts',game/'fonts')
  before={n:hash_file(game/n) for n in manifest['required_existing']}
  args=[str(host),'--client-host','install','--bundle',str(bundle),'--game-dir',str(game),'--no-music-link']+([] if prototype else ['--cjk'])
  subprocess.run(args+['--dry-run'],check=True,timeout=180)
  assert before=={n:hash_file(game/n) for n in before},'Preflight changed game DLLs'
  subprocess.run(args,check=True,timeout=180)
  assert {n:hash_file(game/n) for n in before}==manifest['required_existing']
  state=json.loads((game/'localserver/languages/cjk.json').read_text())
  assert state['version']==manifest['version']
  assert 'cjk_enabled = true' in (game/'server.ini').read_text()
  if prototype:assert state['archives']=={'base':'vfs2','hd':'vfs3'}
  subprocess.run(args,check=True,timeout=180)
  assert (game/'vfs.dat').read_bytes()==b'original index fixture'
  assert (game/'vfs.zip').read_bytes()==b'original archive fixture'
  for name,expected in manifest['vanilla_originals'].items():assert hash_file(game/'bak/originals'/name)==expected
  print('PASS: '+('prototype adoption' if prototype else 'clean install')+', read-only preflight, exact DLL output, repeat install and retained originals.',flush=True)
