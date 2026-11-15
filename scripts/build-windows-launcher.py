#!/usr/bin/env python3
"""Cross-compile the native Windows launcher with Zig; no .NET runtime needed."""
import argparse
from pathlib import Path
import subprocess
import os
import tempfile
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--zig', default='zig')
parser.add_argument('--component', choices=('all', 'launcher', 'client'), default='all',
                    help='Build launcher/setup or injected client independently; all preserves release builds')
parser.add_argument('--output-dir', type=Path, default=Path('build'))
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
version = (root/'VERSION').read_text().strip()
if not re.fullmatch(r'\d+\.\d+\.\d+(?:-(?:beta\.)?[1-9]\d*)?', version):
    raise ValueError('Invalid VERSION')
output = args.output_dir if args.output_dir.is_absolute() else root/args.output_dir
output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ)
env.setdefault('ZIG_GLOBAL_CACHE_DIR', str(Path(tempfile.gettempdir())/'reriff-zig-global'))
env.setdefault('ZIG_LOCAL_CACHE_DIR', str(Path(tempfile.gettempdir())/'reriff-zig-local'))
resource = output/'as-invoker.res'
if args.component in ('all', 'launcher'):
    subprocess.run([os.sys.executable,str(root/'scripts/build-ui-assets.py'),'--output',str(output/'ui-assets')],check=True,env=env)
    def rc_path(path): return '"'+str(path).replace('\\','/')+'"'
    content=(root/'NativeLauncher/app.rc').read_text().replace('"as-invoker.manifest"',rc_path(root/'NativeLauncher/as-invoker.manifest')).replace('RERIFF_ICON_PATH',rc_path(output/'ui-assets/reriff.ico')).replace('RERIFF_LOGO_PATH',rc_path(output/'ui-assets/reriff-logo.bmp')).replace('RERIFF_BUTTON_LOGO_PATH',rc_path(output/'ui-assets/reriff-logo-button.bmp'))
    rc=output/'app.rc';rc.write_text(content)
    subprocess.run([args.zig,'rc','/fo',str(resource),str(rc)],check=True,env=env)
    subprocess.run([args.zig,'cc','-target','x86-windows-gnu','-municode','-Wl,--subsystem,windows',
                    '-O2','-Wall','-Wextra','-Werror','-DRERIFF_VERSION="'+version+'"',str(root/'NativeLauncher/launcher.c'),str(resource),
                    '-o',str(output/'ReRiff-Launcher.exe'),'-lcomctl32','-lwininet','-lshell32','-luser32','-lgdi32','-ladvapi32','-static'],check=True,env=env)
    subprocess.run([args.zig,'cc','-target','x86-windows-gnu','-municode','-Wl,--subsystem,windows',
                    '-O2','-Wall','-Wextra','-Werror',str(root/'NativeLauncher/updater.c'),str(resource),
                    '-o',str(output/'ReRiff-Updater.exe'),'-lcomctl32','-lshell32','-luser32','-lgdi32','-static'],check=True,env=env)
    subprocess.run([args.zig,'cc','-target','x86-windows-gnu','-municode','-Wl,--subsystem,windows',
                    '-O2','-Wall','-Wextra','-Werror','-DRERIFF_VERSION="'+version+'"',str(root/'NativeLauncher/installer.c'),str(resource),
                    '-o',str(output/'ReRiff-Setup.exe'),'-lcomdlg32','-lcomctl32','-lshell32','-luser32','-lgdi32','-lwininet','-ladvapi32','-static'],check=True,env=env)
if args.component in ('all', 'client'):
    subprocess.run([args.zig,'cc','-target','x86-windows-gnu','-shared','-O2','-Wall','-Wextra','-Werror',
                    '-DRERIFF_VERSION="'+version+'"',str(root/'ExperimentalMenu/engine_menu.c'),'-o',str(output/'ReRiff.Notify.dll'),
                    '-luser32','-lgdi32','-ladvapi32','-static'],check=True,env=env)
