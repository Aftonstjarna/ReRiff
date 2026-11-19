"""Font mod packages, legacy migration, ownership and rollback regressions."""
import argparse, configparser, hashlib, io, json, os
from pathlib import Path
import subprocess, tempfile, zipfile
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--host',type=Path,required=True);p.add_argument('--wine');a=p.parse_args()
sha=lambda b:hashlib.sha256(b).hexdigest()
def archive(packs):
    out=io.BytesIO()
    with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
        for pack in packs:z.writestr(f'graphics/fonts/reriff/{pack}/test/batchtextures.brres',b'texture '+pack.encode())
    return out.getvalue()
def snapshot(game):return {str(p.relative_to(game)):sha(p.read_bytes()) for p in game.rglob('*') if p.is_file() and '.bak' not in p.parts}
with tempfile.TemporaryDirectory(prefix='reriff fonts ü ') as temp:
    root=Path(temp)
    def path(p):return 'Z:'+str(p.resolve()).replace('/','\\') if a.wine else str(p.resolve())
    def call(game,pack='cjk',action='install',extra=(),success=True):
        result=root/'result.ini';result.unlink(missing_ok=True)
        cmd=([a.wine] if a.wine else [])+[str(a.host.resolve()),'--language-component',action,'--game-dir',path(game),'--pack',pack,'--result',path(result),*extra]
        run=subprocess.run(cmd,capture_output=True,text=True,errors="replace",timeout=90)
        assert result.exists(),run.stdout+run.stderr
        ini=configparser.ConfigParser(interpolation=None);ini.read(result,encoding='utf-16')
        assert (run.returncode==0)==success and (ini['result']['ok']=='true')==success,(run.returncode,dict(ini['result']),run.stderr)
        return dict(ini['result'])
    assets={};metrics={}
    for pack in ('cjk','latin'):
        names=('jp.rrcf','jp64.rrcf','jp96.rrcf','OFL.txt') if pack=='cjk' else ('latin32.rrcf','latin64.rrcf','latin96.rrcf','OFL.txt')
        metrics[pack]={f'fonts/noto-{pack}/{n}':b'metrics '+n.encode() for n in names}
        files={'archives/combined.dat':b'package index','archives/combined.zip':archive([pack]),**metrics[pack]}
        asset=root/f'{pack}.zip'
        with zipfile.ZipFile(asset,'w') as z:
            for name,data in files.items():z.writestr(name,data)
            z.writestr('component.json',json.dumps({'schema':2,'version':'test','files':{n:sha(b) for n,b in files.items()},'legacy':{}}))
        assets[pack]=('--asset',path(asset),'--sha256',sha(asset.read_bytes()))
    for mode in ('fresh','root','flat','moved'):
        game=root/mode;game.mkdir();(game/'vfs.dat').write_bytes(b'stock');(game/'vfs.zip').write_bytes(b'stock zip')
        (game/'client.ini').write_text('[languages]\ncjk_enabled = true\nlatin_enabled = false\n')
        if mode!='fresh':
            shared={'vfs1.dat':b'old index','vfs1.zip':archive(['cjk','latin'])}
            if mode in ('flat','moved'):(game/'mods').mkdir();(game/'mods/.reriff-vfs').write_text('ReRiff VFS directory version 1\n')
            for name,data in shared.items():
                dest=game/(f'mods/noto-cjk-font/vfs{Path(name).suffix}' if mode=='moved' else f'mods/{name}' if mode=='flat' else name)
                dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
            for pack in metrics:
                state={'schema':1,'version':'legacy','archives':{'combined':'vfs1'},'files':{n:sha(b) for n,b in {**shared,**metrics[pack]}.items()}}
                for n,b in metrics[pack].items():
                    dest=game/n;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(b)
                dest=game/f'localserver/languages/{pack}.json';dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(json.dumps(state))
            before=snapshot(game);stage=root/f'{mode}-stage'
            call(game,action='stage',extra=('--stage',path(stage),'--latin-enable','false'))
            assert snapshot(game)==before
            assert json.loads((stage/'.reriff-language-removals.json').read_text())
            # Destination conflicts stop migration before modifying anything.
            conflict=game/'mods/noto-latin-font/noto-latin/OFL.txt';conflict.parent.mkdir(parents=True,exist_ok=True);conflict.write_bytes(b'user-owned')
            before=snapshot(game);call(game,success=False);assert snapshot(game)==before;conflict.unlink()
            call(game)
        else:
            for pack in metrics:call(game,pack,extra=assets[pack])
        for pack in metrics:
            state=json.loads((game/f'localserver/languages/{pack}.json').read_text());stem=f'mods/noto-{pack}-font/vfs'
            assert state['schema']==2 and state['archives']=={'combined':stem}
            for n,digest in state['files'].items():assert sha((game/n).read_bytes())==digest
            with zipfile.ZipFile(game/(stem+'.zip')) as z:assert all(f'/reriff/{pack}/' in n for n in z.namelist())
        assert (game/'vfs.dat').read_bytes()==b'stock' and (game/'vfs.zip').read_bytes()==b'stock zip'
        assert not (game/'vfs1.zip').exists() and not (game/'mods/vfs1.zip').exists()
        latin=snapshot(game/'mods/noto-latin-font');call(game,extra=assets['cjk']);assert snapshot(game/'mods/noto-latin-font')==latin
        saved_pack=snapshot(game/'mods/noto-cjk-font')
        assert call(game,action='disable')['enabled']=='false'
        assert not (game/'mods/noto-cjk-font/vfs.zip').exists()
        assert snapshot(game/'mods/.disabled/noto-cjk-font')==saved_pack
        assert (game/'mods/.disabled').is_dir()
        assert call(game,action='disable')['enabled']=='false'
        conflict=game/'mods/noto-cjk-font/vfs.zip';conflict.write_bytes(b'user mod')
        before=snapshot(game);call(game,success=False);assert snapshot(game)==before;conflict.unlink()
        assert call(game)['enabled']=='true'
        assert snapshot(game/'mods/noto-cjk-font')==saved_pack
        assert not (game/'mods/.disabled/noto-cjk-font/vfs.zip').exists()
        damaged=game/'mods/noto-cjk-font/vfs.dat';saved=damaged.read_bytes();damaged.write_bytes(b'damaged')
        before=snapshot(game);call(game,extra=assets['cjk'],success=False);assert snapshot(game)==before;damaged.write_bytes(saved)
        print('PASS:',mode,'migration, isolated packs, ownership, staging and enable/disable',flush=True)
    game=root/'broken';game.mkdir();(game/'mods/noto-cjk-font/noto-cjk/OFL.txt').mkdir(parents=True)
    call(game,extra=assets['cjk'],success=False);assert not (game/'mods/noto-cjk-font/vfs.zip').exists();assert not (game/'localserver/languages/cjk.json').exists()
    game=root/'together';game.mkdir();call(game,extra=(*assets['cjk'],'--latin-enable','true','--latin-asset',assets['latin'][1],'--latin-sha256',assets['latin'][3]))
    assert (game/'mods/noto-cjk-font/vfs.zip').exists() and (game/'mods/noto-latin-font/vfs.zip').exists()
    print('PASS: failed-write rollback and simultaneous selections',flush=True)

    # Near-fresh install: deleting mods leaves receipts, but checkboxes reinstall
    # selected packs and clear the absent, unselected pack without resurrecting it.
    import shutil
    shutil.rmtree(game/'mods')
    before=snapshot(game);stage=root/'removed-stage'
    call(game,action='stage',extra=(*assets['cjk'],'--stage',path(stage),'--latin-enable','false'))
    assert snapshot(game)==before
    assert (stage/'mods/noto-cjk-font/vfs.zip').is_file()
    assert not (stage/'mods/noto-latin-font').exists()
    assert 'localserver/languages/latin.json' in json.loads((stage/'.reriff-language-removals.json').read_text())
    call(game,extra=(*assets['cjk'],'--latin-enable','false'))
    assert (game/'mods/noto-cjk-font/vfs.zip').is_file()
    assert not (game/'localserver/languages/latin.json').exists()
    call(game,'latin',extra=assets['latin'])
    shutil.rmtree(game/'mods')
    call(game,extra=(*assets['cjk'],'--latin-enable','true','--latin-asset',assets['latin'][1],'--latin-sha256',assets['latin'][3]))
    assert (game/'mods/noto-cjk-font/vfs.zip').is_file() and (game/'mods/noto-latin-font/vfs.zip').is_file()
    print('PASS: removed mods tree, dry staging, selected packs and stale receipt cleanup',flush=True)
