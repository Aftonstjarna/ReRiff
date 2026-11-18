"""Exercise the shared VFS migration command on Windows or Linux."""
import argparse,json,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--host',type=Path,required=True);p.add_argument('--wine');a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='reriff mods ü ') as tmp:
    root=Path(tmp);game=root/'game';game.mkdir();stage=root/'stage';stage.mkdir()
    def path(x):return 'Z:'+str(x.resolve()).replace('/','\\') if a.wine else str(x.resolve())
    def snapshot():return {f.relative_to(game).as_posix():f.read_bytes() for f in game.rglob('*') if f.is_file()}
    def call(ok=True):
        before=snapshot();r=subprocess.run(([a.wine] if a.wine else [])+[str(a.host.resolve()),'--mods-vfs','--game-dir',path(game),'--stage',path(stage)],capture_output=True,text=True,timeout=90)
        assert (r.returncode==0)==ok,(r.returncode,r.stdout,r.stderr)
        assert snapshot()==before,'Planning must not change installed files'
    (game/'vfs.dat').write_bytes(b'stock');(game/'vfs.zip').write_bytes(b'stock zip')
    (game/'vfs1.dat').write_bytes(b'index');call(False)
    (game/'vfs1.zip').write_bytes(b'zip');call()
    assert (stage/'mods/vfs1.dat').read_bytes()==b'index'
    assert set(json.loads((stage/'.reriff-mods-removals.json').read_text()))=={'vfs1.dat','vfs1.zip'}
    for f in stage.rglob('*'):
        if f.is_file() and f.name!='.reriff-mods-removals.json':
            target=game/f.relative_to(stage);target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(f.read_bytes())
    for name in json.loads((stage/'.reriff-mods-removals.json').read_text()):(game/name).unlink()
    for f in stage.rglob('*'):
        if f.is_file():f.unlink()
    call();assert json.loads((stage/'.reriff-mods-removals.json').read_text())==[]
    (game/'vfs1.dat').write_bytes(b'conflict');call(False)
    assert (game/'vfs.dat').read_bytes()==b'stock'
print('PASS: mods staging, failure status, no preflight writes, migration, repeat, conflicts and stock preservation')
