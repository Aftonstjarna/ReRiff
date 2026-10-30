#!/usr/bin/env python3
"""Compare song-generation primitives and song spline samples with Game.exe x86.

This checks original machine code, but does not certify full generator parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import pefile

root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'NativeLinux'))
from song_track import OriginalRandom,compose,SongSpline
from splines import f32

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir',type=Path,required=True)
parser.add_argument('--nodes',type=Path,required=True)
args=parser.parse_args()
exe=args.game_dir/'Game.exe';identity=hashlib.sha256(exe.read_bytes()).hexdigest()
if identity!='2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2':raise SystemExit('Unsupported Game.exe hash')
build=root/'build/native-math-reference';build.mkdir(parents=True,exist_ok=True)
(build/'image.bin').write_bytes(pefile.PE(str(exe)).get_memory_mapped_image())
commands=[['objcopy','-I','binary','-O','elf32-i386','-B','i386','image.bin','image.o'],
          ['as','--32',str(root/'NativeLinux/reference/original_math.s'),'-o','harness.o'],
          ['ld','-m','elf_i386','-T',str(root/'NativeLinux/reference/original_math.ld'),'harness.o','image.o','-o','original-math']]
for command in commands:subprocess.run(command,cwd=build,check=True,capture_output=True)
report=dict(executable_sha256=identity,rng_values_compared=0,matrix_products_compared=0,song_spline_samples_compared=0,max_absolute_error=0,
            full_generator_parity=False,note='These are direct original x86 primitive comparisons. Full original generated nodes have not been captured or compared.')
def reference(packet):
    result=subprocess.run([str(build/'original-math')],input=packet,capture_output=True,timeout=5)
    if result.returncode:raise RuntimeError(f'Original reference failed ({result.returncode}); environment may block 32-bit syscalls')
    return result.stdout

def equal(actual,expected,label):
    if actual!=expected:raise AssertionError('Original x86 comparison failed: '+label)

for seed in (0,1,5489,0xffffffff,0x12345678):
    for mode in range(4):
        count=1600;rng=OriginalRandom(seed)
        if mode==0:values=[rng.integer() for _ in range(count)];fmt='I';bounds=struct.pack('<2I',0,0)
        elif mode==1:values=[rng.uniform(-2.5,7.25) for _ in range(count)];fmt='f';bounds=struct.pack('<2f',-2.5,7.25)
        elif mode==2:values=[rng.between(-20,35) for _ in range(count)];fmt='i';bounds=struct.pack('<2i',-20,35)
        else:values=[rng.sign() for _ in range(count)];fmt='f';bounds=bytes(8)
        expected=reference(struct.pack('<4I',4,seed,count,mode)+bounds)
        equal(struct.pack('<'+fmt*count,*values),expected,f'random seed {seed} mode {mode}')
        report['rng_values_compared']+=count
randomizer=random.Random(91)
for case in range(100):
    a=tuple(f32(randomizer.uniform(-20,20)) for _ in range(12));b=tuple(f32(randomizer.uniform(-20,20)) for _ in range(12))
    expected=reference(struct.pack('<4I24f',5,0,0,0,*a,*b))
    equal(struct.pack('<12f',*compose(a,b)),expected,'matrix '+str(case));report['matrix_products_compared']+=1
track=json.loads(args.nodes.read_text());spline=SongSpline(track)
for case in range(100):
    t=f32(randomizer.uniform(1,spline.end-0.001))
    for mode in (1,2):
        points=spline.points;packet=struct.pack('<IIfI6f',mode,len(points),t,0,*([0]*6))+b''.join(struct.pack('<3f',*p) for p in points)
        expected=reference(packet);actual=spline.math.evaluate(spline.packed_points,t,mode==2)
        equal(struct.pack('<3f',*actual),expected,'song spline '+str(case));report['song_spline_samples_compared']+=1
output=args.nodes.with_name('original-primitive-parity.json');output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
