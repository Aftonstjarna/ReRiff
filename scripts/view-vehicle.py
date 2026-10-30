#!/usr/bin/env python3
"""Prepare and open the decoded car in the native SDL3/Vulkan viewer."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir',type=Path)
parser.add_argument('--vehicle',default='alf')
parser.add_argument('--track',action='store_true',help='Inspect a recovered road cross-section with the vehicle and original ramp')
parser.add_argument('--genre',choices=('dance','rock','hiphop'),default='dance',help='Inspection/generation genre; must match the original run for comparison')
parser.add_argument('--overview',action='store_true',help='Frame the entire track instead of starting near the car')
parser.add_argument('--section',choices=('straight','curved','uphill','downhill','banked','combined'),help='Use recovered spline math for a road test section; implies --track')
parser.add_argument('--screenshot',type=Path,help='Save an offscreen Vulkan render as a PPM image instead of opening a window')
parser.add_argument('--frames',type=int,help='Close after this many rendered window frames (smoke test)')
parser.add_argument('--song',help='Generate an original bundled song by ID or title; implies --track')
parser.add_argument('--list-songs',action='store_true',help='List installed bundled song IDs and titles')
parser.add_argument('--reference-nodes',type=Path,help='Validate the generated song against an original node capture')
args=parser.parse_args()
if args.reference_nodes and not args.song:parser.error('--reference-nodes requires --song')
args.track=args.track or args.section is not None or args.song is not None
game=args.game_dir
if game is None:
    for candidate in (Path.home()/'.local/share/Steam/steamapps/common/Riff Racer',Path.home()/'.steam/steam/steamapps/common/Riff Racer'):
        if (candidate/'vfs.zip').is_file():game=candidate;break
if game is None or not (game/'vfs.zip').is_file():
    parser.error('Supply --game-dir pointing to the installed Riff Racer directory')
sys.path.insert(0,str(root/'NativeLinux'))
from song_analysis import catalog
if args.list_songs:
    for entry in catalog(game):print(f"{entry['id']:12} {entry['title']} — {entry['artist']}")
    raise SystemExit(0)
if args.song:
    entries=[e for e in catalog(game) if args.song.casefold() in (e['id'].casefold(),e['title'].casefold())]
    if len(entries)!=1:parser.error('Unknown bundled song; use --list-songs')
    args.song=entries[0]['id']
    if args.section:parser.error('--song and --section are mutually exclusive')
if args.frames is not None and args.frames<1:parser.error('--frames must be positive')
if not args.vehicle.isascii() or not args.vehicle.isalnum() or not args.vehicle.islower():parser.error('Invalid vehicle identifier')
output=root/'analysis/native-port'/('track-'+args.genre+'-'+args.vehicle if args.track else 'vehicle-'+args.vehicle)
if args.section and args.section!='straight':output=output.with_name(output.name+'-'+args.section)
if args.song:output=output.with_name('song-'+args.song.lower()+'-'+args.genre+'-'+args.vehicle)
try:
    subprocess.run([sys.executable,str(root/'NativeLinux/setup-viewer.py')],check=True)
    if args.song or (args.section and args.section!='straight'):subprocess.run(['make','-C',str(root/'NativeLinux'),'track-math'],check=True)
    exporter=root/'NativeLinux'/('track_scene.py' if args.track else 'assets.py')
    export_command=[sys.executable,str(exporter),'--game-dir',str(game.resolve()),'--vehicle',args.vehicle,'--output',str(output)]
    if args.track:export_command+=['--genre',args.genre]
    if args.section:export_command+=['--section',args.section]
    if args.song:export_command+=['--song',args.song]
    if args.reference_nodes:export_command+=['--reference-nodes',str(args.reference_nodes.resolve())]
    subprocess.run(export_command,check=True)
    subprocess.run(['make','-C',str(root/'NativeLinux'),'viewer'],check=True)
    command=[str(root/'build/native-linux/reriff-viewer'),str(output/('scene.rmesh' if args.track else 'vehicle.rmesh'))]
    report=json.loads((output/'scene-manifest.json').read_text()) if args.track else {}
    if args.track and not args.overview:
        focus=report.get('viewer_focus',[0,0,1]);distance=report.get('viewer_distance',42)
        command+=['--focus',','.join(str(v) for v in focus),'--distance',str(distance)]
        for car in report.get('vehicles',[])[1:]:
            point=list(car['position']);point[1]+=1
            command+=['--focus',','.join(str(v) for v in point)]
    if 'viewer_elevation' in report:command+=['--elevation',str(report['viewer_elevation'])]
    if args.screenshot:command+=['--screenshot',str(args.screenshot.resolve())]
    if args.frames:command+=['--frames',str(args.frames)]
    subprocess.run(command,check=True)
except (subprocess.CalledProcessError,OSError) as error:
    raise SystemExit(str(error))
