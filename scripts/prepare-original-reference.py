#!/usr/bin/env python3
"""Prepare a disposable DLL-form copy for isolated Microsoft CLR reference tests.

Only PE/CLR header fields change. All original method bodies remain intact.
Requires pefile; output is restricted to the project's ignored build directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import pefile

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'NativeLinux'))
from song_analysis import SUPPORTED_GAME_SHA256


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=ROOT/'build/song-reference/Game.reference.dll')
    args=parser.parse_args()
    output=args.output.resolve()
    if not output.is_relative_to((ROOT/'build').resolve()):parser.error('Reference output must be inside the project build directory')
    original=(args.game_dir/'Game.exe').read_bytes()
    identity=hashlib.sha256(original).hexdigest()
    if identity!=SUPPORTED_GAME_SHA256:parser.error('Unsupported original executable hash')
    pe=pefile.PE(data=original)
    data=bytearray(original);patches=[]
    fields=(
        (pe.FILE_HEADER.get_field_absolute_offset('Characteristics'),'<H',pe.FILE_HEADER.Characteristics|0x2000,'Mark diagnostic copy as DLL'),
        (pe.OPTIONAL_HEADER.get_field_absolute_offset('AddressOfEntryPoint'),'<I',0,'Disable native process entry'),
        (pe.get_offset_from_rva(pe.OPTIONAL_HEADER.DATA_DIRECTORY[14].VirtualAddress)+20,'<I',0,'Disable managed process entry'),
    )
    changed=set()
    for offset,fmt,value,reason in fields:
        patches.append(dict(offset=offset,before=struct.unpack_from(fmt,data,offset)[0],after=value,reason=reason))
        struct.pack_into(fmt,data,offset,value);changed.update(range(offset,offset+struct.calcsize(fmt)))
    assert all(a==b or i in changed for i,(a,b) in enumerate(zip(original,data)))
    output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(data)
    report=dict(original_sha256=identity,reference_sha256=hashlib.sha256(data).hexdigest(),patches=patches,method_bodies_changed=False)
    output.with_name('reference-header-patches.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Prepared '+str(output)+'; original installation unchanged')


if __name__=='__main__':main()
