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
    stem='mods/reriff-assets/vfs'
    state=plan(root/'fresh');assert state['schema']==2 and state['stem']==stem
    assert not (game/'mods').exists()
    # Exact unowned prototypes can migrate; unrelated aliases are retained.
    (game/'vfs4.dat').write_bytes(dat.read_bytes());(game/'vfs4.zip').write_bytes(zip_path.read_bytes())
    assert plan(root/'adopt')['stem']==stem
    assert set(json.loads((root/'adopt/.reriff-artwork-removals.json').read_text()))=={'vfs4.dat','vfs4.zip'}
    (game/'vfs4.dat').unlink();(game/'vfs4.zip').unlink()
    assert plan(root/'reserved',reserved='vfs2.dat,vfs2.zip')['stem']==stem
    (game/'VFS0002.DAT').write_bytes(b'unrelated mod');(game/'VFS0002.ZIP').write_bytes(b'unrelated zip')
    assert plan(root/'collision')['stem']==stem
    # Legacy owned root and flat pairs both retire into the named folder.
    for flat in (False,True):
        state={'schema':1,'stem':'vfs3','files':{'vfs3.dat':sha(dat.read_bytes()),'vfs3.zip':sha(zip_path.read_bytes())}}
        record=game/'localserver/artwork/core.json';record.parent.mkdir(parents=True,exist_ok=True);record.write_text(json.dumps(state))
        folder=game/'mods' if flat else game;folder.mkdir(exist_ok=True)
        if flat:(folder/'.reriff-vfs').write_text('ReRiff VFS directory version 1\n')
        (folder/'vfs3.dat').write_bytes(dat.read_bytes());(folder/'vfs3.zip').write_bytes(zip_path.read_bytes())
        target=root/f'legacy-{flat}';assert plan(target)['stem']==stem
        assert set(json.loads((target/'.reriff-artwork-removals.json').read_text()))=={('mods/' if flat else '')+'vfs3'+ext for ext in ('.dat','.zip')}
        (folder/'vfs3.zip').write_bytes(b'changed');plan(root/f'changed-{flat}',False)
        (folder/'vfs3.dat').unlink();(folder/'vfs3.zip').unlink();record.unlink()
    # A manual move with unchanged hashes is adopted and supports upgrades.
    record.write_text(json.dumps(state))
    for suffix,archive_file in (('.dat',dat),('.zip',zip_path)):
        target=game/(stem+suffix);target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(archive_file.read_bytes())
    assert plan(root/'manual')['schema']==2
    assert not (root/'manual/.reriff-artwork-removals.json').exists()
    record.unlink()
    for suffix in ('.dat','.zip'):(game/(stem+suffix)).unlink()
    # Unowned folder conflicts must never be replaced.
    target=game/(stem+'.dat');target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(b'user file')
    plan(root/'unowned',False);target.unlink()
    for item in (root/'fresh').rglob('*'):
        if item.is_file():
            target=game/item.relative_to(root/'fresh');target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(item.read_bytes())
    assert plan(root/'repeat')['stem']==stem
    texture.write_bytes(b'updated texture');dat,zip_path=build_volume(source,root/'assets','vfs2')
    assert plan(root/'upgrade')['stem']==stem
    plan(root/'integrity',False,dat_hash='0'*64)
    plan(root/'conflict',False,reserved=stem+'.dat')
    (game/(stem+'.dat')).write_bytes(b'user modification');plan(root/'modified',False)
    (game/(stem+'.dat')).unlink();plan(root/'missing',False)
    assert (game/'VFS0002.DAT').read_bytes()==b'unrelated mod'
    assert (game/'vfs.dat').read_bytes()==b'original' and (game/'vfs.zip').read_bytes()==b'original zip'
print('Core artwork planner: named mod, legacy migration, repeat, upgrade, integrity, conflicts and stock preservation passed.')
