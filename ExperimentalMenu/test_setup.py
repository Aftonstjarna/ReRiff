"""Real vanilla/modified DLL setup tests in disposable game directories."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
GAME = Path('/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer')
SETUP = ROOT/'ExperimentalMenu/setup_client.py'
DOTNET = '/tmp/reriff-dotnet/dotnet'


class ClientSetup(unittest.TestCase):
    def fixture(self, vanilla):
        temporary = tempfile.TemporaryDirectory(prefix='reriff-fresh-menu-')
        self.addCleanup(temporary.cleanup)
        game = Path(temporary.name)
        shutil.copy2(GAME/'Game.exe', game/'Game.exe')
        for name in ('ParseOctane.dll', 'Parse.NetFx45.dll'):
            shutil.copy2((ROOT/'backups/client' if vanilla else GAME)/name, game/name)
        if not vanilla:
            (game/'ReRiff.Notify.dll').write_bytes(b'current notification DLL preserved by backup')
            (game/'ReRiff.ClientConfig.dll').write_bytes(b'current helper preserved by backup')
        (game/'server.ini').write_bytes(b'[server]\nshow_server_popup=false\nfallback_url=\n; keep this\n')
        before = {path.name: path.read_bytes() for path in game.iterdir() if path.is_file()}
        return game, before

    def run_setup(self, game, *args):
        subprocess.run(['python3', str(SETUP), '--game-dir', str(game), *args], check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    def inspect(self, game):
        report = game/'inspection.json'
        subprocess.run([DOTNET, str(ROOT/'build/patcher/Patcher.dll'), '--inspect-client', str(game), str(report)], check=True)
        result = json.loads(report.read_text()); report.unlink(); return result

    def test_vanilla_install_and_exact_restore(self):
        game, before = self.fixture(True)
        self.run_setup(game)
        self.assertTrue((game/'bak/experimental-engine-menu').is_dir())
        self.assertTrue(list((game/'bak/experimental-engine-menu').glob('client-*.json')))
        self.assertEqual(self.inspect(game), {'endpoint': True, 'offline': True, 'news': True})
        self.assertTrue((game/'ReRiff.Notify.dll').is_file())
        self.assertEqual((game/'server.ini').read_bytes(), before['server.ini'])
        self.run_setup(game, '--restore')
        self.assertEqual({p.name: p.read_bytes() for p in game.iterdir() if p.is_file()}, before)
        self.assertFalse((game/'localserver/client-version.txt').exists())

    def test_modified_client_preserves_endpoint_and_restores_helpers(self):
        game, before = self.fixture(False)
        self.run_setup(game)
        self.assertEqual((game/'ParseOctane.dll').read_bytes(), before['ParseOctane.dll'])
        self.assertEqual(self.inspect(game), {'endpoint': True, 'offline': True, 'news': True})
        self.run_setup(game, '--restore')
        self.assertEqual({p.name: p.read_bytes() for p in game.iterdir() if p.is_file()}, before)

    def test_write_failure_rolls_back_changed_files(self):
        spec = importlib.util.spec_from_file_location('menu_setup', SETUP)
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); game = root/'game'; game.mkdir()
            (game/'first').write_bytes(b'original')
            real = module.atomic
            def fail_once(path, data):
                if path == game/'second':
                    raise OSError('simulated interrupted write')
                real(path, data)
            with patch.object(module, 'atomic', side_effect=fail_once):
                with self.assertRaises(OSError):
                    module.transact(game, {'first': b'changed', 'second': b'new'}, root/'backups', root/'record.json')
            self.assertEqual((game/'first').read_bytes(), b'original')
            self.assertFalse((game/'second').exists())
            self.assertFalse((root/'record.json').exists())

    def test_backend_extension_preserves_unrelated_code_and_restores(self):
        game, _ = self.fixture(False)
        backend = game/'localserver/server'; backend.mkdir(parents=True)
        original = (GAME/'localserver/server/riffapi-local.py').read_bytes()
        original += b'\n# existing custom backend changes stay intact\n'
        (backend/'riffapi-local.py').write_bytes(original)
        (backend/'libreriff-catalog.so').write_bytes(b'keep existing song identity engine')
        command = ['python3', str(ROOT/'ExperimentalMenu/setup_news.py'), '--game-dir', str(game)]
        subprocess.run(command, check=True, stdout=subprocess.PIPE, text=True)
        self.assertTrue((backend/'riffapi-local.py').read_bytes().endswith(b'# existing custom backend changes stay intact\n'))
        self.assertEqual((backend/'libreriff-catalog.so').read_bytes(), b'keep existing song identity engine')
        subprocess.run(command+['--restore'], check=True, stdout=subprocess.PIPE, text=True)
        self.assertEqual((backend/'riffapi-local.py').read_bytes(), original)
        self.assertFalse((backend/'libreriff-news.so').exists())

    def test_trial_upgrade_keeps_initial_backup_and_refuses_external_changes(self):
        spec = importlib.util.spec_from_file_location('menu_upgrade', SETUP)
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); game = root/'game'; game.mkdir()
            (game/'one').write_bytes(b'original')
            record = root/'record.json'; backups = root/'bak'
            module.transact(game, {'one': b'first trial'}, backups, record)
            module.transact(game, {'one': b'fixed trial', 'two': b'new file'}, backups, record, preserve_record=True)
            info = json.loads(record.read_text())
            self.assertEqual((backups/(info['before']['one']+'.bin')).read_bytes(), b'original')
            self.assertIsNone(info['before']['two'])
            (game/'two').write_bytes(b'outside change')
            with self.assertRaises(ValueError):
                module.transact(game, {'one': b'overwrite'}, backups, record, preserve_record=True)
            self.assertEqual((game/'one').read_bytes(), b'fixed trial')


if __name__ == '__main__':
    unittest.main()
