#!/usr/bin/env python3
"""Compare the port with isolated original x86 math, not the Windows game loop.

Requires pefile, GNU binutils and an x86 Linux kernel permitting 32-bit syscalls.
Generated original code stays in ignored build/ and analysis/ directories.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys

import pefile

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'NativeLinux'))
from splines import NativeMath

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir',type=Path,required=True)
parser.add_argument('--cases',type=int,default=400)
args=parser.parse_args()
if not 1<=args.cases<=10000:parser.error('--cases must be 1..10000')
executable=args.game_dir/'Game.exe'
identity=hashlib.sha256(executable.read_bytes()).hexdigest()
if identity!='2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2':
    raise SystemExit('Reference addresses apply only to the supported original Game.exe hash')
output=root/'build/native-math-reference';output.mkdir(parents=True,exist_ok=True)
pe=pefile.PE(str(executable))
(output/'image.bin').write_bytes(pe.get_memory_mapped_image())
subprocess.run(['objcopy','-I','binary','-O','elf32-i386','-B','i386','image.bin','image.o'],cwd=output,check=True)
subprocess.run(['as','--32',str(root/'NativeLinux/reference/original_math.s'),'-o','harness.o'],cwd=output,check=True)
shutil.copyfile(root/'NativeLinux/reference/original_math.ld',output/'link.ld')
subprocess.run(['ld','-m','elf_i386','-T','link.ld','harness.o','image.o','-o','original-math'],cwd=output,check=True)
subprocess.run(['make','-C',str(root/'NativeLinux'),'track-math'],check=True)
native=NativeMath();randomizer=random.Random(703)
report=dict(executable_sha256=identity,cases=args.cases,comparisons=args.cases*3,
            bitwise_matches=0,max_absolute_error=0,
            note='Original x86 math called directly. Only imported _libm_sse2_sqrt_precise is replaced by SSE2 sqrtsd for finite nonnegative inputs.')
for case in range(args.cases):
    points=[tuple(randomizer.uniform(-200,200) for _ in range(3)) for _ in range(9)]
    t=randomizer.choice([-1.,0.,1.,1.0001,2.,5.999,6.,100.,randomizer.uniform(1,6)])
    forward=tuple(randomizer.uniform(-2,2) for _ in range(3));up=tuple(randomizer.uniform(-2,2) for _ in range(3))
    special=[((0,0,0),(0,0,0)),((0,0,1),(0,0,1)),((1e-7,0,0),(0,1,0)),((0,0,1),(0,0,-1))]
    if case<len(special):forward,up=special[case]
    for mode in (1,2,3):
        packet=struct.pack('<IIfI6f',mode,len(points),t,0,*forward,*up)+b''.join(struct.pack('<3f',*p) for p in points)
        result=subprocess.run([str(output/'original-math')],input=packet,capture_output=True,timeout=5)
        if result.returncode:
            raise SystemExit(f'Reference exited with {result.returncode}; restricted environments may block 32-bit syscalls. No Windows startup code is used.')
        actual=native.evaluate(points,t,mode==2) if mode!=3 else sum(native.orientation(forward,up),())
        expected=struct.unpack('<%df'%(len(result.stdout)//4),result.stdout)
        error=max(abs(a-b) for a,b in zip(actual,expected))
        report['max_absolute_error']=max(report['max_absolute_error'],error)
        if struct.pack('<%df'%len(actual),*actual)==result.stdout:report['bitwise_matches']+=1
        else:raise SystemExit(f'Math parity mismatch at case {case}, mode {mode}: {actual} versus {expected}')
destination=root/'analysis/native-port/spline-parity.json'
destination.parent.mkdir(parents=True,exist_ok=True)
destination.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
