#!/usr/bin/env python3
"""Capture original song-generation output in an existing Microsoft CLR prefix.

The installed game is read-only. A DLL-form reference with unchanged method
bodies, original config bytes and an offline file bridge run inside build/.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'NativeLinux'))
from song_analysis import load_bundled,SUPPORTED_GAME_SHA256
from song_track import SongGenerator


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def windows(path):return 'Z:'+str(path.resolve()).replace('/','\\')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir',type=Path,default=Path.home()/'.local/share/Steam/steamapps/common/Riff Racer')
    parser.add_argument('--song',default='Rolling On')
    parser.add_argument('--genre',choices=('dance','rock','hiphop'),default='dance')
    parser.add_argument('--prefix',type=Path,default=ROOT/'build/song-reference/dotnet452-proton1102-win64')
    parser.add_argument('--wine',type=Path,default=Path.home()/'.local/share/Steam/steamapps/common/Proton 11.0/files/bin/wine')
    parser.add_argument('--dotnet',type=Path,default=Path(shutil.which('dotnet') or '/tmp/reriff-dotnet/dotnet'))
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    if digest(args.game_dir/'Game.exe')!=SUPPORTED_GAME_SHA256:parser.error('Unsupported original Game.exe hash')
    clr=args.prefix/'drive_c/windows/Microsoft.NET/Framework/v4.0.30319/clr.dll'
    core=clr.with_name('mscorlib.dll')
    if not clr.exists() or not core.exists():parser.error('An existing prefix with Microsoft .NET Framework is required; see DOTNET452-INVESTIGATION.md')
    if (args.prefix/'dosdevices/z:').resolve()!=Path('/'):parser.error('The capture host requires the standard Z: mapping to /')
    entry,profile,analysis,analysis_hash=load_bundled(args.game_dir,args.song)
    output=(args.output or ROOT/'analysis/native-port'/('original-'+entry['id'].lower()+'-'+args.genre)).resolve()
    if not output.is_relative_to((ROOT/'analysis').resolve()):parser.error('Capture output must be inside the project analysis directory')
    output.mkdir(parents=True,exist_ok=True)
    build=ROOT/'build/song-reference';inputs=build/('input-'+entry['id'].lower()+'-'+args.genre);inputs.mkdir(parents=True,exist_ok=True)
    for name,data in (('profile',profile),('analysis',analysis)):(inputs/(name+'.json')).write_text(json.dumps(data)+'\n')
    (inputs/'genre.txt').write_text(args.genre+'\n')
    (inputs/'timing.txt').write_text('\n'.join(str(entry[k]) for k in ('audible_start','first_beat','mid_beat'))+'\n')
    config_hashes={}
    with zipfile.ZipFile(args.game_dir/'vfs.zip') as archive:
        for name in ('segments_PC.cfg','sections_PC.cfg','visualSections.cfg'):
            path='scripts/config/'+name;data=archive.read(path);(inputs/name).write_bytes(data);config_hashes[path]=hashlib.sha256(data).hexdigest()
    subprocess.run([sys.executable,str(ROOT/'scripts/prepare-original-reference.py'),'--game-dir',str(args.game_dir)],check=True)
    env=os.environ.copy();env['DOTNET_ROOT']=str(args.dotnet.resolve().parent)
    sdks=subprocess.check_output([str(args.dotnet),'--list-sdks'],env=env,text=True).splitlines()
    if not sdks:raise RuntimeError('A .NET SDK is needed to compile the diagnostic host')
    version,base=sdks[-1].split(' ',1);compiler=Path(base.strip('[]'))/version/'Roslyn/bincore/csc.dll'
    host=build/'OriginalSongCapture.exe';source=ROOT/'NativeLinux/reference/OriginalSongCapture.cs'
    subprocess.run([str(args.dotnet),str(compiler),'-nologo','-unsafe','-target:exe','-platform:x86','-nostdlib','-r:'+str(core),'-out:'+str(host),str(source)],env=env,check=True)
    env.update(WINEPREFIX=str(args.prefix.resolve()),WINEDEBUG='-all',WINEDLLOVERRIDES='winemenubuilder.exe=d;mscoree=n')
    # Let Wine use the existing prefix's architecture, rather than converting it.
    env.pop('WINEARCH',None)
    log=output/'capture.log'
    with log.open('w') as stream:
        result=subprocess.run([str(args.wine),windows(host),windows(build/'Game.reference.dll'),windows(args.game_dir),windows(inputs),windows(output)],env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=120)
    text=log.read_text(errors='replace')
    if result.returncode or 'RESULT=SONG_CAPTURE_OK' not in text:raise RuntimeError('Original capture failed; inspect '+str(log))
    raw=(output/'original-nodes.bin').read_bytes()
    if not raw or len(raw)%244:raise ValueError('Invalid original node-vector byte length')
    nodes=[]
    for i in range(len(raw)//244):
        offset=i*244
        nodes.append(dict(index=i,position=struct.unpack_from('<3f',raw,offset+172),up=struct.unpack_from('<3f',raw,offset+148),
                          widths=struct.unpack_from('<2f',raw,offset+208),walls=struct.unpack_from('<2f',raw,offset+216),visible=raw[offset+228],
                          flags=struct.unpack_from('<I',raw,offset+240)[0],segment=struct.unpack_from('<i',raw,offset+224)[0],section=struct.unpack_from('<i',raw,offset+232)[0]))
    reference=dict(source='original-game',capture_method='Offline Microsoft CLR host; original generation methods unchanged; read-only config I/O bridge',
                   executable_sha256=SUPPORTED_GAME_SHA256,song_id=profile['song']['id'],genre=args.genre,analysis_sha256=analysis_hash,
                   config_sha256={k:v for k,v in config_hashes.items() if k.endswith(('segments_PC.cfg','sections_PC.cfg'))},nodes=nodes)
    reference_path=output/'original-nodes.json';reference_path.write_text(json.dumps(reference,indent=2)+'\n')
    native=SongGenerator(args.game_dir,profile,analysis,args.genre).generate();native['analysis_sha256']=analysis_hash
    (output/'linux-nodes.json').write_text(json.dumps(native,indent=2)+'\n')
    spec=importlib.util.spec_from_file_location('song_comparison',ROOT/'scripts/compare-song-track.py');comparison=importlib.util.module_from_spec(spec);spec.loader.exec_module(comparison)
    report=comparison.compare(native,reference)
    captured_segments=[line.split('\t') for line in (output/'original-segments.tsv').read_text().splitlines()]
    segment_mismatches=[]
    for i,(a,b) in enumerate(zip(native['segments'],captured_segments)):
        exact_params=all(x==struct.unpack('<f',struct.pack('<f',float(y)))[0] for x,y in zip(a['params'],b[5:9]))
        if (a['start'],a['end'],a['type'])!=tuple(map(int,b[1:4])) or not exact_params:segment_mismatches.append(i)
    report.update(segment_counts=[len(native['segments']),len(captured_segments)],segment_mismatches=segment_mismatches)
    report['passed']=report['passed'] and not segment_mismatches and len(native['segments'])==len(captured_segments)
    report.update(reference_sha256=digest(reference_path),captured_at_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        native_host_source_sha256=digest(source),header_changes=json.loads((build/'reference-header-patches.json').read_text()),original_installation_hash_unchanged=digest(args.game_dir/'Game.exe')==SUPPORTED_GAME_SHA256)
    (output/'comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));print('Capture directory: '+str(output))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
