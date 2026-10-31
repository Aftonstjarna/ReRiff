"""Test the real Wine stop button path against isolated owned/unrelated processes."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--wine', required=True)
parser.add_argument('--prefix', required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='reriff-stop-test-') as temporary:
    game = Path(temporary)/'game with spaces'
    server = game/'localserver'
    (server/'server').mkdir(parents=True)
    for name in ('server-control.py', 'stop-server-host.sh'):
        shutil.copyfile(ROOT/'scripts'/name, server/name)
    runner = server/'trace-server.py'
    runner.write_text('import time\nfrom pathlib import Path\nPath(__file__).with_name("ready").touch()\nwhile True: time.sleep(1)\n')
    owned = subprocess.Popen([sys.executable, str(runner), '--server-dir', str(server/'server')])
    unrelated = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(120)'])
    try:
        deadline = time.monotonic()+5
        while time.monotonic()<deadline and not (server/'ready').exists():
            time.sleep(0.05)
        assert (server/'ready').exists()
        subprocess.run(['/usr/bin/distrobox-host-exec', 'xvfb-run', '-a', 'env', 'WINEPREFIX='+args.prefix, 'WINEDEBUG=-all', args.wine,
                        str(ROOT/'build/test-stop-server.exe'), 'Z:'+str(game)], check=True, timeout=45)
        owned.wait(timeout=3)
        assert unrelated.poll() is None
        print('PASS: bridge completion confirmed, owned process stopped, unrelated process preserved, repeat stop succeeds')
    finally:
        for process in (owned, unrelated):
            if process.poll() is None:
                process.terminate()
            process.wait()
