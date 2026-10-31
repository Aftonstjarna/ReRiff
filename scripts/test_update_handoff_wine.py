"""Run the real native parent-exit/update/relaunch sequence in disposable Wine."""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--wine', required=True)
parser.add_argument('--prefix', required=True)
parser.add_argument('--platform', choices=['linux', 'windows'], default='linux')
args = parser.parse_args()
spec = importlib.util.spec_from_file_location('updater_fixture', ROOT/'scripts/test_client_update.py')
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)
case = fixture.ClientUpdateTest()
case.setUp()
try:
    archive, digest = case.package(args.platform, launcher=(ROOT/'build/test-update-relaunch.exe').read_bytes())
    shutil.copyfile(ROOT/'scripts/apply-client-update.py', case.work/'apply.py')
    if args.platform == 'linux':
        fixture.update.prepare(archive, digest, case.work, case.game, 'linux')
        command = '"C:\\windows\\system32\\start.exe" /wait /unix /usr/bin/python3 "'+str(case.work/'apply.py')+'" --work "'+str(case.work)+'"'
    else:
        runtime = case.game/'localserver/windows-python'
        shutil.rmtree(runtime)
        shutil.copytree(ROOT/'build/release/windows/python', runtime)
        subprocess.run(['/usr/bin/distrobox-host-exec', 'xvfb-run', '-a', 'env', 'WINEPREFIX='+args.prefix, 'WINEDEBUG=-all', args.wine,
                        str(runtime/'python.exe'), 'Z:'+str(case.work/'apply.py'), '--prepare', '--work', 'Z:'+str(case.work),
                        '--archive', 'Z:'+str(archive), '--sha256', digest, '--game-dir', 'Z:'+str(case.game), '--platform', 'windows'], check=True, timeout=30)
        assert (case.work/'prepared.json').exists()
        command = '"Z:'+str(case.work/'runtime/python.exe')+'" "Z:'+str(case.work/'apply.py')+'" --work "Z:'+str(case.work)+'"'
    (case.work/'apply-command.txt').write_bytes(command.encode('utf-16-le'))
    subprocess.run(['/usr/bin/distrobox-host-exec', 'xvfb-run', '-a', 'env', 'WINEPREFIX='+args.prefix, 'WINEDEBUG=-all', args.wine,
                    str(ROOT/'build/test-update-parent.exe'), 'Z:'+str(ROOT/'build/ReRiff-Updater.exe'),
                    'Z:'+str(case.work), 'Z:'+str(case.game)], check=True, timeout=30)
    deadline = time.monotonic()+30
    while time.monotonic()<deadline and not (case.game/'reopened').exists():
        time.sleep(0.1)
    assert (case.work/'success').is_file(), (case.work/'update.log').read_text() if (case.work/'update.log').exists() else 'No update result'
    assert (case.game/'reopened').is_file(), 'Launcher did not reopen'
    assert (case.game/'server.ini').read_bytes() == b'external settings\n'
    assert (case.game/'localserver/save.json').read_bytes() == b'saved scores'
    print('PASS: native updater waited for parent exit, applied '+args.platform+' package, preserved settings/data and relaunched')
finally:
    case.doCleanups()
