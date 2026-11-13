"""Exercise independent font downloads sharing one owned VFS pair."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zipfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--host',type=Path,required=True)
parser.add_argument('--cjk',type=Path,required=True)
parser.add_argument('--latin',type=Path,required=True)
a=parser.parse_args();host=a.host.resolve();assets={'cjk':a.cjk.resolve(),'latin':a.latin.resolve()}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def call(game,pack,action='install',extra=()):
    asset=assets[pack]
    subprocess.run([str(host),'--language-component',action,'--game-dir',str(game),'--pack',pack,*(['--asset',str(asset),'--sha256',sha(asset)] if action=='install' else []),*extra],check=True,capture_output=True,timeout=300)

def validate(game):
    states=[json.loads((game/f'localserver/languages/{p}.json').read_text()) for p in assets]
    assert states[0]['archives']==states[1]['archives']=={'combined':'vfs1'}
    assert sorted(p.name for p in game.glob('vfs*'))==['vfs.dat','vfs.zip','vfs1.dat','vfs1.zip']
    for state in states:
        for name,digest in state['files'].items():assert sha(game/name)==digest,name
    raw=(game/'vfs1.dat').read_bytes();magic,nodes,count,strings,crc=struct.unpack_from('<5I',raw)
    assert magic==0x0300ffee and len(raw)==28+nodes*8+count*28+strings
    with zipfile.ZipFile(game/'vfs1.zip') as z:
        assert z.testzip() is None
        assert crc==sum(e.CRC for e in z.infolist())&0xffffffff
        assert any('/cjk/' in e for e in z.namelist()) and any('/latin/' in e for e in z.namelist())
        central=0
        for i,e in enumerate(z.infolist()):
            assert struct.unpack_from('<7I',raw,28+nodes*8+i*28)==(2,e.compress_type,e.compress_size,e.file_size,len(e.filename),e.header_offset,central)
            central+=46+len(e.filename)+len(e.extra)+len(e.comment)
    assert (game/'vfs.dat').read_bytes()==b'original index'
    assert (game/'vfs.zip').read_bytes()==b'original archive'

with tempfile.TemporaryDirectory(prefix='font-packs-',dir=Path(__file__).resolve().parents[1]/'build') as temp:
    root=Path(temp)
    for order in (('cjk','latin'),('latin','cjk')):
        game=root/order[0];game.mkdir();(game/'vfs.dat').write_bytes(b'original index');(game/'vfs.zip').write_bytes(b'original archive')
        for pack in order:call(game,pack)
        validate(game)
        call(game,'latin','disable');config=(game/'server.ini').read_text();assert 'latin_enabled = false' in config and 'cjk_enabled = true' in config
        # Corrupted owned archives must be refused without changing anything.
        path=game/'vfs1.dat';saved=path.read_bytes();path.write_bytes(saved+b'bad')
        before={str(p.relative_to(game)):sha(p) for p in game.rglob('*') if p.is_file()}
        try:call(game,'latin')
        except subprocess.CalledProcessError:pass
        else:raise AssertionError('Accepted a changed owned archive')
        assert before=={str(p.relative_to(game)):sha(p) for p in game.rglob('*') if p.is_file()}
        path.write_bytes(saved)
        print('PASS: shared VFS, original preservation, independent disable and damaged-file refusal:',order,flush=True)
    game=root/'together';game.mkdir()
    asset=assets['cjk'];latin=assets['latin']
    subprocess.run([str(host),'--language-component','install','--game-dir',str(game),'--asset',str(asset),'--sha256',sha(asset),'--latin-enable','true','--latin-asset',str(latin),'--latin-sha256',sha(latin)],check=True,capture_output=True,timeout=300)
    assert sorted(p.name for p in game.glob('vfs*'))==['vfs1.dat','vfs1.zip']
    assert json.loads((game/'localserver/languages/cjk.json').read_text())['archives']==json.loads((game/'localserver/languages/latin.json').read_text())['archives']
    print('PASS: simultaneous installer selections share one VFS pair.',flush=True)
