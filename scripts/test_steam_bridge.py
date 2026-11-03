"""Exercise the actual Steam bridge with a fake Proton command, without launching games."""
from pathlib import Path
import json
import os
import subprocess
import tempfile
import shutil
import unittest

ROOT = Path(__file__).resolve().parents[1]
class SteamBridgeTest(unittest.TestCase):
    def test_flags_and_game_arguments(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            capture = root/'argv.json'
            proton = root/'fake proton'
            proton.write_text('#!/usr/bin/env python3\nimport json,os,sys\nopen(os.environ["RERIFF_CAPTURE"],"w").write(json.dumps(sys.argv[1:]))\n')
            proton.chmod(0o755)
            game = str(root/'game folder/Game.exe')
            bridge = root/'ReRiff-Launcher.sh'
            shutil.copy2(ROOT/'scripts/steam-launcher.sh', bridge)
            bridge.chmod(0o755)
            for flags, suffix in [(['--skip-launcher','--test-auto-update'], []), ([], ['--test-auto-update','--no-auto-update']), (['--show-launcher'], [])]:
                with self.subTest(flags=flags, suffix=suffix):
                    subprocess.run([str(bridge), *flags, '--', str(proton), 'waitforexitandrun', game,
                                    'space value', 'quote"value', '音楽', *suffix], check=True, env={**os.environ, 'RERIFF_CAPTURE':str(capture)})
                    argv = json.loads(capture.read_text())
                    self.assertEqual(argv[:2], ['waitforexitandrun', str(root/'game folder/ReRiff-Launcher.exe')])
                    split = argv.index('--')
                    self.assertEqual(argv[2:split], flags+suffix)
                    self.assertEqual(argv[split+1:], [game, 'space value', 'quote"value', '音楽'])
    def test_no_proton_wrapper(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            launcher = root/'ReRiff-Launcher.exe'
            launcher.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
            launcher.chmod(0o755)
            game = str(root/'Game.exe')
            result = subprocess.run(['bash',str(ROOT/'scripts/steam-launcher.sh'),'--test-auto-update','--',game],check=True,text=True,capture_output=True)
            self.assertEqual(result.stdout.splitlines(), ['--test-auto-update','--',game])
    def test_missing_game_is_rejected(self):
        result = subprocess.run(['bash',str(ROOT/'scripts/steam-launcher.sh'),'--test-auto-update','--','not-the-game'],capture_output=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn(b'Supply the Steam game command',result.stderr)

if __name__ == '__main__':unittest.main()
