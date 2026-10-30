#!/usr/bin/env python3
"""Cross-compile the native Windows launcher with Zig; no .NET runtime needed."""
import argparse
from pathlib import Path
import subprocess
import os
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--zig', default='zig')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
(root/'build').mkdir(exist_ok=True)
env = dict(os.environ)
env.setdefault('ZIG_GLOBAL_CACHE_DIR', str(Path(tempfile.gettempdir())/'reriff-zig-global'))
env.setdefault('ZIG_LOCAL_CACHE_DIR', str(Path(tempfile.gettempdir())/'reriff-zig-local'))
subprocess.run([args.zig,'cc','-target','x86-windows-gnu','-municode','-Wl,--subsystem,windows',
                '-O2','-Wall','-Wextra','-Werror',str(root/'NativeLauncher/launcher.c'),
                '-o',str(root/'build/ReRiff-Launcher.exe'),'-lwininet','-lshell32','-luser32','-lgdi32','-static'],check=True,env=env)
subprocess.run([args.zig,'cc','-target','x86-windows-gnu','-shared','-O2','-Wall','-Wextra','-Werror',
                str(root/'NativeLauncher/notification.c'),'-o',str(root/'build/ReRiff.Notify.dll'),
                '-luser32','-lgdi32','-static'],check=True,env=env)
subprocess.run([args.zig,'cc','-target','x86-windows-gnu','-municode','-Wl,--subsystem,windows',
                '-O2','-Wall','-Wextra','-Werror',str(root/'NativeLauncher/installer.c'),
                '-o',str(root/'build/ReRiff-Setup.exe'),'-lcomdlg32','-luser32','-static'],check=True,env=env)
