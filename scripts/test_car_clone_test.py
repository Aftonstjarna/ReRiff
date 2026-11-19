#!/usr/bin/env python3
"""Validate generated test archives without opening a game or changing saves."""
import argparse
import importlib.util
from pathlib import Path
import struct
import zipfile
spec=importlib.util.spec_from_file_location('builder',Path(__file__).with_name('build-car-clone-test.py'))
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)


def evaluate(raw):
    size=raw[8];at=12
    def uint(n=4):
        nonlocal at
        v=int.from_bytes(raw[at:at+n],'little');at+=n;return v
    def string():
        nonlocal at
        n=uint(size);v=raw[at:at+n];at+=n
        return v[:-1].decode('ascii') if n else ''
    string();uint();uint()
    nonlocal_header=raw[at:at+4];at+=4
    assert nonlocal_header[:3]==bytes([0,0,2])
    instructions=[uint() for _ in range(uint())]
    constants=[]
    for _ in range(uint()):
        kind=uint(1)
        if kind==4:constants.append(string())
        else:
            assert kind==3
            constants.append(struct.unpack_from('<f',raw,at)[0]);at+=4
    assert [uint() for _ in range(4)]==[0]*4 and at==len(raw)
    registers={};globals={}
    for ins in instructions:
        op=ins&63;a=(ins>>6)&255;c=(ins>>14)&511;d=(ins>>23)&511;bx=ins>>14
        if op==1:registers[a]=constants[bx]
        elif op==2:registers[a]=bool(d)
        elif op==10:registers[a]={}
        elif op==9:registers[a][constants[d-256] if d>=256 else registers[d]]=constants[c-256] if c>=256 else registers[c]
        elif op==34:
            assert c>0 and d>0
            for i in range(1,d+1):registers[a][(c-1)*50+i]=registers[a+i]
        elif op==7:globals[constants[bx]]=registers[a]
        else:assert op==30 and d==1
    return globals


def values(node):
    if node['type']!=2:return bool(node['value']) if node['name']=='unlockedAtStart' else node['value']
    array=node['name']=='default' or all(not n['name'] for n in node['value'])
    children=node['value']
    first=0 if children and children[0]['type']==0 and children[0]['value']=='' else 1
    return {(i+first if array else n['name']):values(n) for i,n in enumerate(children)}


def main():
    p=argparse.ArgumentParser();p.add_argument('package',type=Path);p.add_argument('reference',type=Path);a=p.parse_args()
    with zipfile.ZipFile(a.package/'mod/vfs.zip') as z, zipfile.ZipFile(a.reference/'vfs.zip') as stock:
        for stem in ['vehicleSelect','unlocks','unlocks_PC','paint','paint_PC']:
            path='scripts/config/'+stem.lower()
            cfg=b.parse_config(z.read(path+'.cfg'));expected={n['name']:values(n) for n in cfg}
            for suffix in ['.luac','.luac64']:assert evaluate(z.read(path+suffix))==expected
            original=b.parse_config(stock.read('scripts/config/'+stem+'.cfg'))
            if stem.startswith('paint'):
                before=b.named(original,'Paints')['value'];after=b.named(cfg,'Paints')['value']
                assert [n for n in before if n['name']!='dod']==[n for n in after if n['name']!='dod']
                clone=b.named(after,'dod')['value'];assert len(clone)==2
                native=evaluate(stock.read('scripts/config/'+stem+'.luac'))['Paints']['alf']
                generated=evaluate(z.read(path+'.luac'))['Paints']['dod']
                assert set(native)==set(range(1,10))
                assert set(generated)=={1,2}
                for pack in generated.values():
                    for color in ['col0','col1','col2','specCol','alloysCol0','alloysCol1']:
                        assert set(pack[color])==set(native[1][color])=={1,2,3}
            else:
                assert b.named(cfg,'carNames')['value'][:-1]==b.named(original,'carNames')['value']
                assert b.named(cfg,'carNames')['value'][-1]['value']=='dod'
                assert len(b.named(cfg,'carNames')['value'])-1==16
                lua=evaluate(z.read(path+'.luac'))
                assert list(lua['default'].keys())==list(range(1,17))
                # Original vehicleSelect Lua's final NEWTABLE/GETGLOBAL/SETLIST order.
                order=['alf','bui','cad','che','cor','fab','hon','mer','mot','pha','rol','rs1','vau','f1r','pl1','dod']
                assert list(lua['default'].values())==[lua[n] for n in order]
                if stem.startswith('unlocks'):
                    assert lua['carUnlocks'][16]['unlockedAtStart'] is False
        for n in stock.namelist():
            lower=n.lower()
            if '/alf/' in lower or '/decals/alf_' in lower:
                target=lower.replace('/alf/','/dod/').replace('/alf_','/dod_')
                assert z.read(target)==stock.read(n)
    print('PASS: 5 configs roundtrip; both Lua architectures match config semantics; stock car order/paints retained; Pulse model bytes identical.')

if __name__=='__main__':main()
