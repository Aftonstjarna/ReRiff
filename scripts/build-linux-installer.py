#!/usr/bin/env python3
"""Build a portable native GTK 3 installer (runtime-loaded, glibc 2.28 baseline)."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--zig', default='zig')
args = parser.parse_args()
version = (root/'VERSION').read_text().strip()
env = dict(os.environ)
env.setdefault('ZIG_GLOBAL_CACHE_DIR', '/tmp/reriff-zig-global')
env.setdefault('ZIG_LOCAL_CACHE_DIR', '/tmp/reriff-zig-local')
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', 'gtk+-3.0'], text=True))
# Ubuntu's broad multiarch include path would shadow Zig's target libc bits
# with the runner's newer libc headers (notably the time64 definitions). Keep
# that path for third-party headers, after the portable target's system headers.
flags = [part for flag in flags for part in
         (['-idirafter', flag[2:]] if flag == '-I/usr/include/x86_64-linux-gnu' else [flag])]

(root/'build').mkdir(exist_ok=True)
subprocess.run([args.zig, 'cc', '-target', 'x86_64-linux-gnu.2.28', '-O2', '-Wall', '-Wextra',
                '-Werror', '-Wno-deprecated-declarations', '-DRERIFF_VERSION="'+version+'"',
                *flags, str(root/'LinuxInstaller/installer.c'), '-ldl',
                '-o', str(root/'build/ReRiff-Setup')], check=True, env=env)
