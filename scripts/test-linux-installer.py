#!/usr/bin/env python3
"""Exercise the actual GTK installer and child-process flow on a virtual display."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--zig', default='zig')
args = parser.parse_args()
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', 'gtk+-3.0'], text=True))
# Ubuntu's broad multiarch include path would shadow Zig's target libc bits
# with the runner's newer libc headers (notably the time64 definitions). Keep
# that path for third-party headers, after the portable target's system headers.
flags = [part for flag in flags for part in
         (['-idirafter', flag[2:]] if flag == '-I/usr/include/x86_64-linux-gnu' else [flag])]

env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR='/tmp/reriff-zig-global', ZIG_LOCAL_CACHE_DIR='/tmp/reriff-zig-local')
exe = root/'build/test-linux-installer'
subprocess.run([args.zig, 'cc', '-target', 'x86_64-linux-gnu.2.28', '-O2', '-Wall', '-Wextra', '-Werror',
                '-Wno-deprecated-declarations', *flags, str(root/'LinuxInstaller/test_installer.c'),
                '-ldl', '-o', str(exe)], check=True, env=env)
with tempfile.TemporaryDirectory(prefix='reriff-gui-test-') as temp:
    work = Path(temp)
    game = work/'Riff Racer ü with spaces'
    game.mkdir()
    (game/'Game.exe').write_bytes(b'fixture only')
    (game/'server.ini').write_text('[diagnostics]\ncjk_render_probe=1\n')
    invalid = work/'Wrong game folder'
    invalid.mkdir()
    (work/'installer.py').write_text('''import argparse, pathlib, time, sys
p=argparse.ArgumentParser(); p.add_argument('--bundle'); p.add_argument('--game-dir'); p.add_argument('--music-dir'); p.add_argument('--cjk',action='store_true'); p.add_argument('--no-cjk',action='store_true'); p.add_argument('--no-music-link',action='store_true'); a=p.parse_args()
assert a.bundle
game=pathlib.Path(a.game_dir)
time.sleep(0.4)
marker=game/'fixture-runs'
if not marker.exists():
    assert a.no_music_link and not a.music_dir and a.no_cjk and not a.cjk
    marker.write_text('1')
    print('Expected fixture failure', flush=True)
    sys.exit(7)
assert a.music_dir==a.game_dir and not a.no_music_link and a.cjk and not a.no_cjk
print('GUI install succeeded: '+str(game), flush=True)
''')
    # Fixture executable follows the native client's install argument contract.
    runtime = work/'payload/localserver/ReRiff-ClientHost'
    runtime.parent.mkdir(parents=True)
    runtime.write_text('#!/bin/sh\n[ "$1" = --client-host ] && [ "$2" = install ] || exit 2\nshift 2\nexec '+shlex.quote(sys.executable)+' '+shlex.quote(str(work/'installer.py'))+' "$@"\n')
    runtime.chmod(0o755)
    # No test process may touch the real user's home or logs.
    (work/'home').mkdir()
    env.update(HOME=str(work/'home'), XDG_CONFIG_HOME=str(work/'config'),
               XDG_CACHE_HOME=str(work/'cache'), XDG_DATA_HOME=str(work/'data'),
               XDG_STATE_HOME=str(work/'state'), GSETTINGS_BACKEND='memory',
               NO_AT_BRIDGE='1', GDK_BACKEND='x11', GTK_MODULES='')
    env.pop('WAYLAND_DISPLAY', None)
    subprocess.run(['xvfb-run', '-a', str(exe), str(work), str(game), str(invalid)], check=True, env=env, timeout=20)
    assert len(list((game/'logs').glob('installer-*.log'))) == 2
