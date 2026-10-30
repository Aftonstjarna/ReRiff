#!/usr/bin/env python3
"""Compare exported Linux nodes with an independently captured original node dump.

Reference JSON requires source='original-game', executable_sha256, song_id,
genre, and a nodes array. Each node needs position, up, widths, walls, visible,
and flags, read from the original gaNode record (244 bytes). See SONG-TRACKS.md.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

HASH='2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'


def compare(native,reference,tolerance=0.01):
    if reference.get('source')!='original-game' or reference.get('executable_sha256')!=HASH:
        raise ValueError('Reference must be an independently captured original-game dump with the supported executable hash')
    if native['song']['id']!=reference.get('song_id') or native['genre']!=reference.get('genre'):
        raise ValueError('Reference song ID and genre must match the Linux export')
    for field in ('analysis_sha256','config_sha256'):
        if field in reference and native.get(field)!=reference[field]:
            raise ValueError('Reference input identity differs: '+field)
    actual=native['nodes'];expected=reference['nodes']
    if not isinstance(actual,list) or not isinstance(expected,list) or not actual or not expected:
        raise ValueError('Both captures must contain a nonempty node array')
    result=dict(passed=len(actual)==len(expected),node_counts=[len(actual),len(expected)],tolerance=tolerance,
                max_position_error=0.,max_up_error=0.,max_width_error=0.,max_wall_error=0.,flag_mismatches=[],visibility_mismatches=[])
    for i,(a,b) in enumerate(zip(actual,expected)):
        for field,stat,size in (('position','max_position_error',3),('up','max_up_error',3),('widths','max_width_error',2),('walls','max_wall_error',2)):
            if len(a[field])!=size or len(b[field])!=size:raise ValueError('Invalid reference vector length')
            if not all(isinstance(v,(int,float)) and math.isfinite(v) for v in b[field]):raise ValueError('Invalid reference number')
            error=math.sqrt(sum((x-y)**2 for x,y in zip(a[field],b[field])))
            result[stat]=max(result[stat],error)
            if error>tolerance:result['passed']=False
        if a['visible']!=b['visible']:result['visibility_mismatches'].append(i);result['passed']=False
        if a['flags']!=b['flags']:result['flag_mismatches'].append(i);result['passed']=False
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native',type=Path);parser.add_argument('reference',type=Path)
    parser.add_argument('--tolerance',type=float,default=.01)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    if not math.isfinite(args.tolerance) or not 0<args.tolerance<=1:parser.error('Tolerance must be in (0,1]')
    try:
        result=compare(json.loads(args.native.read_text()),json.loads(args.reference.read_text()),args.tolerance)
        result['reference_sha256']=hashlib.sha256(args.reference.read_bytes()).hexdigest()
        if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps(result,indent=2));return 0 if result['passed'] else 1
    except (ValueError,KeyError,TypeError) as error:parser.error(str(error))

if __name__=='__main__':raise SystemExit(main())
