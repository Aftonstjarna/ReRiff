"""Run the real native parent-exit/update/relaunch sequence in disposable Wine."""
import argparse
import hashlib
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--wine', required=True)
parser.add_argument('--prefix', required=True)
parser.add_argument('--platform', choices=['linux', 'windows'], default='linux')
parser.add_argument('--automatic', action='store_true')
parser.add_argument('--updater', type=Path, default=ROOT/'build/ReRiff-Updater.exe')
args = parser.parse_args()
spec = importlib.util.spec_from_file_location('updater_fixture', ROOT/'scripts/test_client_update.py')
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)
case = fixture.ClientUpdateTest()
case.setUp()
# The updater now owns a progress window after the launcher exits. Keep the
# test display alive for the whole handoff, not just the short-lived parent.
reader, writer = os.pipe()
display_process = subprocess.Popen(['Xvfb', '-displayfd', str(writer), '-screen', '0', '1024x768x24', '-nolisten', 'tcp', '-ac'],
                                  pass_fds=(writer,), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
os.close(writer)
with os.fdopen(reader) as stream:
    display = ':'+stream.readline().strip()
assert display != ':', 'Temporary display did not start'
wine_prefix = ['/usr/bin/distrobox-host-exec', 'env', 'DISPLAY='+display, 'WINEPREFIX='+args.prefix, 'WINEDEBUG=-all']
try:
    archive, digest = case.package(args.platform, launcher=(ROOT/'build/test-update-relaunch.exe').read_bytes())
    shutil.copyfile(ROOT/'scripts/apply-client-update.py', case.work/'apply-client-update.py')
    if args.platform == 'linux':
        fixture.update.prepare(archive, digest, case.work, case.game, 'linux')
        command = '"C:\\windows\\system32\\start.exe" /wait /unix /usr/bin/python3 "'+str(case.work/'apply-client-update.py')+'" --work "'+str(case.work)+'"'
    else:
        runtime = case.game/'localserver/windows-python'
        shutil.rmtree(runtime)
        shutil.copytree(ROOT/'build/release/windows/python', runtime)
        subprocess.run(wine_prefix+[args.wine,
                        str(runtime/'python.exe'), 'Z:'+str(case.work/'apply-client-update.py'), '--prepare', '--work', 'Z:'+str(case.work),
                        '--archive', 'Z:'+str(archive), '--sha256', digest, '--game-dir', 'Z:'+str(case.game), '--platform', 'windows'], check=True, timeout=30)
        assert (case.work/'prepared.json').exists()
        command = '"Z:'+str(case.work/'runtime/python.exe')+'" "Z:'+str(case.work/'apply-client-update.py')+'" --work "Z:'+str(case.work)+'"'
    (case.work/'apply-command.txt').write_bytes(command.encode('utf-16-le'))
    if args.automatic:
        resume = '"Z:'+str(case.game/'ReRiff-Launcher.exe')+'" --skip-launcher --no-auto-update -- "Z:'+str(case.game/'Game.exe')+'" "space value"'
        (case.work/'resume-command.txt').write_bytes(resume.encode('utf-16-le'))
    subprocess.run(wine_prefix+[args.wine,
                    str(ROOT/'build/test-update-parent.exe'), 'Z:'+str(args.updater),
                    'Z:'+str(case.work), 'Z:'+str(case.game)], check=True, timeout=30)
    deadline = time.monotonic()+30
    while time.monotonic()<deadline and not (case.game/'reopened').exists():
        time.sleep(0.1)
    assert (case.work/'success').is_file(), (case.work/'update.log').read_text() if (case.work/'update.log').exists() else 'No update result'
    assert (case.game/'reopened').is_file(), 'Launcher did not reopen'
    resumed = (case.game/'reopened').read_text()
    if args.automatic:
        assert '--skip-launcher --no-auto-update' in resumed and '"space value"' in resumed, resumed
        notice=(case.game/'localserver/update-notification.ini').read_bytes()
        assert b'9.0.0-2' in notice, notice
    else:
        assert '--show-launcher' in resumed, resumed
        assert not (case.game/'localserver/update-notification.ini').exists()
    assert (case.game/'server.ini').read_bytes() == b'external settings\n'
    assert (case.game/'localserver/save.json').read_bytes() == b'saved scores'
    print('PASS: native updater waited for parent exit, applied '+args.platform+' package, preserved settings/data and relaunched')
finally:
    display_process.terminate()
    display_process.wait(timeout=10)
    case.doCleanups()
