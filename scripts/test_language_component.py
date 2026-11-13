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
    (game/'localserver').mkdir();(game/'localserver/client-channel.ini').write_text('[client]\nchannel=experimental\n')
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
    (game/'bak/versions').mkdir(parents=True);(game/'bak/versions/retained').write_bytes(b'old backup')
    assert call('install','--asset',path(asset),'--sha256',sha)['enabled']=='true'
    assert not (game/'bak').exists() and (game/'.bak/versions/retained').read_bytes()==b'old backup'
    owned=json.loads((game/'localserver/languages/cjk.json').read_text())
    assert owned['archives']=={'base':'vfs2','hd':'vfs3'},owned
    assert (game/'VFS0001.DAT').read_bytes()==b'another mod'
    cfg=(game/'server.ini').read_text();assert cfg.count('[languages]')==1 and 'other = keep' in cfg
    assert call('disable')['enabled']=='false'
    assert (game/'vfs2.zip').read_bytes()==b'base zip'
    assert call('install')['enabled']=='true' # Re-enable entirely offline.
    call('install','--asset',path(asset),'--sha256',sha)
    # Merge both owned volumes without touching the colliding third-party pair.
    combined=root/'combined.zip'
    merged={n:b for n,b in files.items() if not n.startswith('archives/')}
    merged.update({'archives/combined.dat':b'combined dat','archives/combined.zip':b'combined zip'})
    with zipfile.ZipFile(combined,'w') as z:
        for name,data in merged.items():z.writestr(name,data)
        z.writestr('component.json',json.dumps({'schema':2,'version':'merged','files':{n:hash_bytes(b) for n,b in merged.items()},'legacy':{}}))
    merged_sha=hash_bytes(combined.read_bytes())
    call('stage','--asset',path(combined),'--sha256',merged_sha,'--stage',path(root/'staged-upgrade'))
    removals=json.loads((root/'staged-upgrade/.reriff-language-removals.json').read_text())
    assert removals=={'vfs3.dat':hash_bytes(b'hd dat'),'vfs3.zip':hash_bytes(b'hd zip')},removals
    assert (game/'vfs3.zip').read_bytes()==b'hd zip' # Staging is read-only.
    # A failed state write after retiring vfs3 must restore both old pairs.
    if not a.wine:
        state_folder=game/'localserver/languages';state_folder.chmod(0o555)
        try:call('install','--asset',path(combined),'--sha256',merged_sha,success=False)
        finally:state_folder.chmod(0o755)
        assert (game/'vfs2.zip').read_bytes()==b'base zip'
        assert (game/'vfs3.zip').read_bytes()==b'hd zip'
        assert json.loads((state_folder/'cjk.json').read_text())['archives']=={'base':'vfs2','hd':'vfs3'}
    call('install','--asset',path(combined),'--sha256',merged_sha)
    owned=json.loads((game/'localserver/languages/cjk.json').read_text())
    assert owned['archives']=={'combined':'vfs2'},owned
    assert not (game/'vfs3.dat').exists() and not (game/'vfs3.zip').exists()
    assert (game/'VFS0001.DAT').read_bytes()==b'another mod'
    assert (game/'.bak/versions'/(hash_bytes(b'hd zip')+'-vfs3.zip')).read_bytes()==b'hd zip'
    call('install','--asset',path(combined),'--sha256',merged_sha)
    assert call('disable')['enabled']=='false'
    assert call('install')['enabled']=='true'
    call('stage','--asset',path(asset),'--sha256',sha,'--stage',path(root/'staged-split'))
    call('stage','--asset',path(combined),'--sha256',merged_sha,'--stage',path(root/'staged-combined'))
    (game/'vfs2.dat').write_bytes(b'edited by user')
    call('install','--asset',path(combined),'--sha256',merged_sha,success=False)
    assert (game/'vfs2.dat').read_bytes()==b'edited by user'
    # A component write failure must undo all earlier font writes.
    broken=root/'Broken game';broken.mkdir();(broken/'Game.exe').write_bytes(b'fixture');(broken/'server.ini').write_bytes(config)
    (broken/'fonts/noto-cjk/OFL.txt').mkdir(parents=True)
    call('install','--asset',path(asset),'--sha256',sha,success=False,target=broken)
    assert not list(broken.glob('vfs*'))
    assert (broken/'server.ini').read_bytes()==config
    assert not (broken/'localserver/languages/cjk.json').exists()
    stable=root/'Stable game';stable.mkdir();(stable/'Game.exe').write_bytes(b'fixture')
    (stable/'server.ini').write_bytes(config)
    call('install','--asset',path(combined),'--sha256',merged_sha,target=stable)
    assert not (stable/'.bak/versions').exists()
    assert list((stable/'.bak/configs').glob('*-server.ini'))
    call('disable',target=stable)
    assert not (stable/'.bak/versions').exists()
    for i in range(4):
        (stable/'server.ini').write_bytes(config+b'\n# revision '+str(i).encode()+b'\n')
        call('disable',target=stable)
    assert {p.read_bytes() for p in (stable/'.bak/configs').glob('*-server.ini')}=={
        config+b'\n# revision 2\n',config+b'\n# revision 3\n'}
    print('PASS: verified font packages, two-to-one volume migration, retired-file backups, unowned archive collision, config preservation, offline enable/disable, modified-file refusal and rollback.')
