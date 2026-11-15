#!/usr/bin/env python3
"""Exercise the real cross-platform artwork planner without editing game files."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'packaging'))
from native_archive import build_volume
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--host',type=Path,required=True);p.add_argument('--wine');a=p.parse_args()
sha=lambda b:hashlib.sha256(b).hexdigest()
with tempfile.TemporaryDirectory(prefix='reriff artwork ü ') as temp:
    root=Path(temp);game=root/'Game with spaces';game.mkdir()
    (game/'vfs.dat').write_bytes(b'original');(game/'vfs.zip').write_bytes(b'original zip')
    source=root/'source';texture=source/'graphics/reriff/achievements/batchtextures.brres';texture.parent.mkdir(parents=True);texture.write_bytes(b'fixture texture')
    dat,zip_path=build_volume(source,root/'assets','vfs2')
    def path(value):return 'Z:'+str(value.resolve()).replace('/','\\') if a.wine else str(value.resolve())
    def snapshot():return {p.relative_to(game).as_posix():p.read_bytes() for p in game.rglob('*') if p.is_file()}
    def plan(stage,expected=True,reserved='',dat_hash=None):
        before=snapshot()
        command=([a.wine] if a.wine else [])+[str(a.host.resolve()),'--core-artwork','--game-dir',path(game),'--stage',path(stage),'--dat',path(dat),'--dat-sha256',dat_hash or sha(dat.read_bytes()),'--zip',path(zip_path),'--zip-sha256',sha(zip_path.read_bytes()),'--reserved',reserved]
        result=subprocess.run(command,capture_output=True,text=True,timeout=60)
        assert (result.returncode==0)==expected,(command,result.stdout,result.stderr)
        assert snapshot()==before,'Planning changed installed files'
        return json.loads((stage/'localserver/artwork/core.json').read_text()) if expected else None
    state=plan(root/'fresh');assert state['stem']=='vfs2'
    assert not (game/'vfs2.dat').exists()
    (game/'vfs4.dat').write_bytes(dat.read_bytes());(game/'vfs4.zip').write_bytes(zip_path.read_bytes())
    assert plan(root/'adopt')['stem']=='vfs4'
    (game/'vfs4.dat').unlink();(game/'vfs4.zip').unlink()
    # Reserve volumes chosen by optional font staging in this same transaction.
    assert plan(root/'reserved',reserved='vfs2.dat,vfs2.zip')['stem']=='vfs3'
    (game/'VFS0002.DAT').write_bytes(b'unrelated mod');(game/'VFS0002.ZIP').write_bytes(b'unrelated zip')
    assert plan(root/'collision')['stem']=='vfs3'
    for item in (root/'fresh').rglob('*'):
        if item.is_file():
            target=game/item.relative_to(root/'fresh');target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(item.read_bytes())
    # An alias conflict with an existing owned volume is refused too.
    plan(root/'alias',False)
    (game/'VFS0002.DAT').unlink();(game/'VFS0002.ZIP').unlink()
    assert plan(root/'repeat')['stem']=='vfs2'
    texture.write_bytes(b'updated texture');dat,zip_path=build_volume(source,root/'assets','vfs2')
    assert plan(root/'upgrade')['stem']=='vfs2'
    plan(root/'integrity',False,dat_hash='0'*64)
    (game/'vfs2.dat').write_bytes(b'user modification');plan(root/'modified',False)
    (game/'vfs2.dat').unlink();plan(root/'missing',False)
    assert (game/'vfs.dat').read_bytes()==b'original' and (game/'vfs.zip').read_bytes()==b'original zip'
print('Core artwork planner: fresh, font reservation, aliases, upgrade, integrity and ownership checks passed.')
