#!/usr/bin/env python3
"""Exercise uninstall using disposable folders and verified vanilla DLL copies."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
p=argparse.ArgumentParser();p.add_argument('--host',type=Path,required=True);p.add_argument('--vanilla',type=Path,required=True);p.add_argument('--wine');p.add_argument('--running-launcher',type=Path);a=p.parse_args()
originals=json.loads((Path(__file__).resolve().parents[1]/'packaging/original-client.json').read_text())
sha=lambda b:hashlib.sha256(b).hexdigest()
for name,digest in originals.items():assert sha((a.vanilla/name).read_bytes())==digest
with tempfile.TemporaryDirectory(prefix='reriff uninstall ü ') as temp:
    game=Path(temp)/'Game';game.mkdir();backup=game/'.bak/originals';backup.mkdir(parents=True)
    owned={'ReRiff.ClientConfig.dll':b'helper','ReRiff-Launcher.exe':b'launcher','mods/reriff-assets/vfs.dat':b'index','mods/reriff-assets/vfs.zip':b'archive'}
    for name in originals:
        shutil.copyfile(a.vanilla/name,backup/name);owned[name]=b'patched '+name.encode()
    if a.running_launcher: owned['ReRiff-Launcher.exe']=a.running_launcher.read_bytes()
    preserved={'Game.exe':b'original game','client.ini':b'private setting','server.ini':b'private endpoint','localserver/data/profiles.json':b'account data','singleplayer/save0.bin':b'save','mods/custom/vfs.zip':b'user mod','logs/test.log':b'log'}
    for name,data in {**owned,**preserved}.items():
        target=game/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    receipt={'schema':1,'backup_dir':str(game/'.bak'),'files':{n:sha(b) for n,b in owned.items()},'originals':originals}
    if a.wine:receipt['backup_dir']='Z:'+receipt['backup_dir'].replace('/','\\')
    record=game/'localserver/uninstall-state.json';record.write_text(json.dumps(receipt))
    component=game/'localserver/artwork/core.json';component.parent.mkdir(parents=True);component.write_text(json.dumps({'files':{n:sha(b) for n,b in owned.items() if n.startswith('mods/')}}))
    def snapshot():return {p.relative_to(game).as_posix():p.read_bytes() for p in game.rglob('*') if p.is_file()}
    def run(ok=True,dry=False):
        path='Z:'+str(game).replace('/','\\') if a.wine else str(game)
        result=subprocess.run(([a.wine] if a.wine else [])+[str(a.host.resolve()),'--uninstall','--game-dir',path]+(['--dry-run'] if dry else []),capture_output=True,text=True,errors="replace",timeout=60)
        assert (result.returncode==0)==ok,(result.stdout,result.stderr)
    before=snapshot();run(dry=True);assert before==snapshot()
    (backup/'ParseOctane.dll').write_bytes(b'bad backup');before=snapshot();run(False);assert before==snapshot()
    shutil.copyfile(a.vanilla/'ParseOctane.dll',backup/'ParseOctane.dll')
    (game/'ReRiff.ClientConfig.dll').write_bytes(b'user modification');before=snapshot();run(False);assert before==snapshot()
    (game/'ReRiff.ClientConfig.dll').write_bytes(owned['ReRiff.ClientConfig.dll'])
    receipt['files']['singleplayer/save0.bin']=sha(preserved['singleplayer/save0.bin']);record.write_text(json.dumps(receipt));before=snapshot();run(False);assert before==snapshot()
    del receipt['files']['singleplayer/save0.bin'];record.write_text(json.dumps(receipt))
    # A missing optional file is harmless; no missing-file removal is attempted.
    (game/'mods/reriff-assets/vfs.dat').unlink()
    if not a.wine:
        folder=game/'mods/reriff-assets';folder.chmod(0o555);before=snapshot()
        try:run(False);assert before==snapshot(), 'Failed uninstall did not roll back'
        finally:folder.chmod(0o755)
    running=None
    try:
        if a.running_launcher:
            import time
            running=subprocess.Popen([a.wine,str(game/'ReRiff-Launcher.exe')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            time.sleep(3)
            assert running.poll() is None, 'Launcher fixture did not start'
        run()
    finally:
        if running is not None:running.terminate();running.wait(timeout=10)
    for name,digest in originals.items():assert sha((game/name).read_bytes())==digest
    for name,data in preserved.items():assert (game/name).read_bytes()==data,name
    for name in owned:
        if name not in originals:assert not (game/name).exists(),name
    assert list((game/'.bak').glob('uninstall-*'))
    assert not component.exists()
    run(dry=True) # already uninstalled remains safe
print('Uninstall: originals restored; data preserved; modified files, invalid backups and protected paths rejected.')
