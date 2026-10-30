"""Read the game's packed SNG0 analysis cache without modifying original assets."""
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

from assets import need
from splines import f32

MAX_JSON=16*1024*1024
SUPPORTED_GAME_SHA256='2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
CATALOG=Path(__file__).with_name('bundled-songs.json')


def decode_song(data):
    need(20<=len(data)<=2*MAX_JSON,'invalid song cache size')
    magic,c1,c2,u1,u2=struct.unpack_from('<5I',data)
    need(magic==0x534e4730,'expected SNG0 packed song (disk bytes 0GNS)')
    need(0<c1 and 0<c2 and 20+c1+c2==len(data),'invalid packed song lengths')
    def decode(payload,size):
        need(1<=size<=MAX_JSON,'song JSON exceeds size limit')
        decoder=zlib.decompressobj()
        try:raw=decoder.decompress(payload,size+1)
        except zlib.error as error:raise ValueError('invalid song compression') from error
        need(len(raw)==size and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,'invalid song compressed stream or expanded length')
        need(raw.endswith(b'\0') and b'\0' not in raw[:-1],'song JSON must have one terminating NUL')
        def reject(value):raise ValueError('nonfinite song JSON number: '+value)
        result=json.loads(raw[:-1],parse_constant=reject)
        need(isinstance(result,dict),'expected song JSON object')
        return result
    profile=decode(data[20:20+c1],u1);analysis=decode(data[20+c1:],u2)
    validate(profile,analysis)
    return profile,analysis


def validate(profile,analysis):
    song=profile.get('song',{})
    need(isinstance(song,dict) and isinstance(song.get('id'),str) and 0<len(song['id'])<=64,'invalid song ID')
    track=analysis.get('track',{})
    need(isinstance(track,dict),'missing track analysis')
    tempo=track.get('tempo',0) or 120
    need(isinstance(tempo,(int,float)) and math.isfinite(tempo) and 20<=tempo<=400,'unsupported song tempo')
    beats=analysis.get('beats',[]);volumes=analysis.get('beatVols',[])
    need(isinstance(beats,list) and 4<=len(beats)<=50000,'invalid beat count')
    need(isinstance(volumes,list) and len(volumes)==len(beats),'beat/loudness count mismatch')
    for values in (beats,volumes):
        need(all(isinstance(v,(int,float)) and math.isfinite(v) for v in values),'invalid beat/loudness value')
    need(beats[0]>=0 and all(a<b for a,b in zip(beats,beats[1:])),'beat times must increase')
    need(all(0<=v<=1 for v in volumes),'loudness outside normalized range')
    sections=analysis.get('sections',[])
    need(isinstance(sections,list) and 0<len(sections)<=len(beats),'invalid sections')
    previous=-1
    for section in sections:
        need(isinstance(section,dict),'invalid section record')
        for name in ('beat','start','duration','loudness'):
            v=section.get(name)
            need(isinstance(v,(int,float)) and math.isfinite(v) and v>=0,'invalid section '+name)
        beat=math.floor(f32(section['beat']+0.5))
        need(previous<beat<len(beats),'invalid section beat ordering');previous=beat
    need(sections[0]['beat']==0,'first section must start at beat zero')


def catalog(game_dir):
    result=[]
    for entry in json.loads(CATALOG.read_text()):
        path=game_dir/'audio/music/bundled'/(entry['id'].lower()+'.json')
        if path.is_file():result.append(dict(entry,analysis_path=path))
    return result


def load_bundled(game_dir,identifier):
    matches=[e for e in catalog(game_dir) if identifier.casefold() in (e['id'].casefold(),e['title'].casefold())]
    need(len(matches)==1,'unknown bundled song; use --list-songs')
    entry=matches[0];data=entry['analysis_path'].read_bytes();profile,analysis=decode_song(data)
    need(profile['song']['id'].casefold()==entry['id'].casefold(),'song ID differs from bundled table')
    return entry,profile,analysis,hashlib.sha256(data).hexdigest()
