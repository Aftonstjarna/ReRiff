"""Exercise language install ownership, enable/disable and transactional safety."""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import zipfile

p=argparse.ArgumentParser(description=__doc__);p.add_argument('--host',type=Path,required=True);p.add_argument('--wine');a=p.parse_args()
hash_bytes=lambda b:hashlib.sha256(b).hexdigest()
with tempfile.TemporaryDirectory(prefix='reriff languages ü ') as temporary:
    root=Path(temporary);game=root/'Game with spaces';game.mkdir();(game/'Game.exe').write_bytes(b'fixture')
    config=b'[server]\nexternal_url = https://example.invalid\n[languages]\nother = keep\n';(game/'server.ini').write_bytes(config)
    files={'archives/base.dat':b'base dat','archives/base.zip':b'base zip','archives/hd.dat':b'hd dat','archives/hd.zip':b'hd zip',
           **{'fonts/noto-cjk/'+n:b'metrics '+n.encode() for n in ('jp.rrcf','jp64.rrcf','jp96.rrcf')},'fonts/noto-cjk/OFL.txt':b'license fixture'}
    asset=root/'fonts.zip'
    with zipfile.ZipFile(asset,'w') as z:
        for name,data in files.items():z.writestr(name,data)
        z.writestr('component.json',json.dumps({'schema':1,'version':'test','files':{n:hash_bytes(b) for n,b in files.items()},'legacy':{}}))
    sha=hash_bytes(asset.read_bytes())
    def path(value):return 'Z:'+str(value.resolve()).replace('/','\\') if a.wine else str(value.resolve())
    def call(action,*extra,success=True,target=game):
        result=root/'result.ini';result.unlink(missing_ok=True)
        command=([a.wine] if a.wine else [])+[str(a.host.resolve()),'--language-component',action,'--game-dir',path(target),'--result',path(result),*extra]
        run=subprocess.run(command,capture_output=True,text=True,timeout=90)
        assert result.exists(),run.stdout+run.stderr
        state=configparser.ConfigParser(interpolation=None);state.read(result,encoding='utf-16');values=dict(state['result'])
        assert (run.returncode==0)==success,(run.returncode,values,run.stderr)
        assert (values['ok']=='true')==success,values
        return values
    assert call('status')['installed']=='false'
    call('install','--asset',path(asset),'--sha256','0'*64,success=False)
    assert (game/'server.ini').read_bytes()==config
    (game/'VFS0001.DAT').write_bytes(b'another mod');(game/'VFS0001.ZIP').write_bytes(b'other mod zip')
    assert call('install','--asset',path(asset),'--sha256',sha)['enabled']=='true'
    owned=json.loads((game/'localserver/languages/cjk.json').read_text())
    assert owned['archives']=={'base':'vfs2','hd':'vfs3'},owned
    assert (game/'VFS0001.DAT').read_bytes()==b'another mod'
    cfg=(game/'server.ini').read_text();assert cfg.count('[languages]')==1 and 'other = keep' in cfg
    assert call('disable')['enabled']=='false'
    assert (game/'vfs2.zip').read_bytes()==b'base zip'
    assert call('install')['enabled']=='true' # Re-enable entirely offline.
    call('install','--asset',path(asset),'--sha256',sha)
    (game/'vfs2.dat').write_bytes(b'edited by user')
    call('install','--asset',path(asset),'--sha256',sha,success=False)
    assert (game/'vfs2.dat').read_bytes()==b'edited by user'
    # A component write failure must undo all earlier font writes.
    broken=root/'Broken game';broken.mkdir();(broken/'Game.exe').write_bytes(b'fixture');(broken/'server.ini').write_bytes(config)
    (broken/'fonts/noto-cjk/OFL.txt').mkdir(parents=True)
    call('install','--asset',path(asset),'--sha256',sha,success=False,target=broken)
    assert not list(broken.glob('vfs*'))
    assert (broken/'server.ini').read_bytes()==config
    assert not (broken/'localserver/languages/cjk.json').exists()
    print('PASS: verified font package, unowned archive collision, config preservation, offline enable/disable, modified-file refusal and rollback.')
