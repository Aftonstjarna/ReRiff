#!/usr/bin/env python3
"""Stop only the Python backend belonging to this localserver installation."""
import argparse
import os
from pathlib import Path
import signal
import sys
import time


def stamp(pid):
    try:
        parts=Path(f'/proc/{pid}/stat').read_text().rsplit(')',1)[1].split()
        return parts[19] if parts[0]!='Z' else None
    except (OSError,IndexError):
        return None


def owned(pid,root):
    try:
        args=Path(f'/proc/{pid}/cmdline').read_bytes().decode().split('\0')
        runner=str((root/'trace-server.py').resolve())
        return runner in args and '--server-dir' in args and Path(args[args.index('--server-dir')+1]).resolve()==(root/'server').resolve()
    except (OSError,UnicodeError,ValueError,IndexError):
        return False


def stop(root):
    root=root.resolve()
    candidates=[]
    for entry in Path('/proc').iterdir():
        if entry.name.isdigit() and owned(int(entry.name),root):
            identity=stamp(int(entry.name))
            if identity:candidates.append((int(entry.name),identity))
    if not candidates:
        print('No Python backend owned by this installation is running. Docker and other servers were left untouched.')
        return
    for pid,identity in candidates:
        if stamp(pid)==identity and owned(pid,root):os.kill(pid,signal.SIGTERM)
    deadline=time.monotonic()+10
    while time.monotonic()<deadline:
        if all(stamp(pid)!=identity for pid,identity in candidates):
            print('Local Python server stopped.');return
        time.sleep(.1)
    raise RuntimeError('Server is still stopping; no force kill was attempted.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('stop',))
    parser.add_argument('--result',type=Path,help='Write an atomic completion marker for the Wine bridge')
    args=parser.parse_args()
    error_text=None
    try:stop(Path(__file__).resolve().parent)
    except (OSError,RuntimeError) as error:error_text=str(error)
    if args.result:
        temporary=args.result.with_name(args.result.name+'.partial')
        temporary.write_text('error: '+error_text+'\n' if error_text else 'ok\n',encoding='utf-8')
        temporary.replace(args.result)
    if error_text:sys.exit(error_text)
