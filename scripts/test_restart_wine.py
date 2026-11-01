"""Exercise save/restart through a launcher and from a directly started fixture."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--wine',required=True)
parser.add_argument('--prefix',required=True)
parser.add_argument('--inside-host',action='store_true')
args=parser.parse_args()
if not args.inside_host:
    host=['/usr/bin/distrobox-host-exec'] if Path('/usr/bin/distrobox-host-exec').exists() else []
    subprocess.run(host+['xvfb-run','-a',sys.executable,str(Path(__file__).resolve()),
        '--inside-host','--wine',args.wine,'--prefix',args.prefix],check=True)
    raise SystemExit(0)
env=dict(os.environ,WINEPREFIX=args.prefix,WINEDEBUG='-all',
    WINESERVER=str(Path(args.wine).parent/'wineserver'),WINEDLLOVERRIDES='mscoree,mshtml=')
for direct in (False,True):
    with tempfile.TemporaryDirectory(prefix='reriff restart test ') as temporary:
        game=Path(temporary)
        shutil.copy2(root/'build/ReRiff-Launcher.exe',game/'ReRiff-Launcher.exe')
        shutil.copy2(root/'build/experimental-engine-menu/test-restart-game.exe',game/'Game.exe')
        (game/'server.ini').write_text('[server]\nnetworking_enabled=false\n')
        command=[args.wine,str(game/'Game.exe')] if direct else [args.wine,str(game/'ReRiff-Launcher.exe'),'--skip-launcher','--','Z:'+str(game/'Game.exe')]
        with (game/'wine-output.txt').open('w+') as log:
            process=subprocess.Popen(command+['space value','音楽'],env=env,stdout=log,stderr=log)
            deadline=time.monotonic()+40
            while time.monotonic()<deadline and not (game/'restart-complete').exists():time.sleep(0.1)
            if not (game/'restart-complete').exists():
                log.seek(0);raise AssertionError('Restart did not finish: '+log.read())
            process.wait(timeout=10)
        assert (game/'runs.txt').read_text()=='2'
        first=(game/'arguments-1.bin').read_bytes().decode('utf-16-le')
        second=(game/'arguments-2.bin').read_bytes().decode('utf-16-le')
        assert first==second and 'space value' in second and '音楽' in second
        print('PASS:', 'direct game handoff' if direct else 'supervising launcher',
              'saved settings, old process fully exited, exactly one restart, original arguments retained')
