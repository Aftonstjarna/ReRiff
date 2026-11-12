"""Build the optional, redistributable Noto CJK component from pinned sources."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def build(reuse=False):
    subprocess.run([sys.executable,str(ROOT/'scripts/fetch-cjk-fonts.py')],check=True)
    base=load('cjk_component_base',ROOT/'scripts/build-cjk-atlas.py')
    hd=load('cjk_component_hd',ROOT/'scripts/build-cjk-hd-atlas.py')
    archive=load('cjk_component_archive',ROOT/'packaging/native_archive.py')
    target=ROOT/'build/cjk-component';target.mkdir(parents=True,exist_ok=True)
    resources=target/'resources';high=target/'hd-resources';package=target/'package'
    if package.exists():shutil.rmtree(package)
    (package/'archives').mkdir(parents=True)
    (package/'fonts/noto-cjk').mkdir(parents=True)
    source=ROOT/'build/noto-cjk/NotoSansCJK-Regular.ttc'
    for region in base.REGIONS:
        if not reuse or not (resources/f'graphics/fonts/reriff/cjk/{region}/glyphs.rrcf').exists():
            print('Baking',region,32,flush=True);base.bake(source,resources,region)
        shutil.copy2(resources/f'graphics/fonts/reriff/cjk/{region}/glyphs.rrcf',package/f'fonts/noto-cjk/{region}.rrcf')
    for pixels in (64,96):
        if not reuse or not (high/f'graphics/fonts/reriff/cjk/jp{pixels}/glyphs.rrcf').exists():
            print('Baking JP',pixels,flush=True);hd.bake(source,high,'jp',pixels)
        shutil.copy2(high/f'graphics/fonts/reriff/cjk/jp{pixels}/glyphs.rrcf',package/f'fonts/noto-cjk/jp{pixels}.rrcf')
    # Only textures belong in native archives. Metrics/license remain loose files.
    for label,tree in (('base',resources),('hd',high)):
        clean=target/(label+'-textures')
        if clean.exists():shutil.rmtree(clean)
        for path in tree.rglob('batchtextures.brres'):
            dest=clean/path.relative_to(tree);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dest)
        for path in archive.build_volume(clean,target/(label+'-volume'),'vfs1'):
            shutil.copy2(path,package/('archives/'+label+path.suffix))
    pinned=json.loads((ROOT/'packaging/fonts/noto-cjk.json').read_text())
    assert hashlib.sha256((ROOT/'packaging/fonts/OFL-Noto-CJK.txt').read_bytes()).hexdigest()==pinned['files'][1]['sha256'], 'Noto license differs from pinned upstream source'
    shutil.copy2(ROOT/'packaging/fonts/OFL-Noto-CJK.txt',package/'fonts/noto-cjk/OFL.txt')
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    legacy=json.loads((ROOT/'packaging/fonts/legacy-archives.json').read_text())
    meta={'schema':1,'version':(ROOT/'VERSION').read_text().strip(),'files':{p.relative_to(package).as_posix():digest(p) for p in sorted(package.rglob('*')) if p.is_file()},'legacy':legacy}
    (package/'component.json').write_text(json.dumps(meta,indent=2)+'\n')
    output=target/'ReRiff-CJK.zip'
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for path in sorted(package.rglob('*')):
            if path.is_file():z.write(path,path.relative_to(package).as_posix())
    print(output,flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--reuse',action='store_true');args=parser.parse_args();build(args.reuse)
