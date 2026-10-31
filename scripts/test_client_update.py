"""Test the updater's extraction, preflight, settings preservation and rollback."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import tarfile
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('client_update', ROOT/'scripts/apply-client-update.py')
update = importlib.util.module_from_spec(spec)
spec.loader.exec_module(update)


class ClientUpdateTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.game = self.root/'game with spaces'
        (self.game/'localserver').mkdir(parents=True)
        (self.game/'localserver/windows-python').mkdir()
        (self.game/'localserver/windows-python/python.exe').write_bytes(b'existing runtime')
        (self.game/'Game.exe').write_bytes(b'game')
        (self.game/'ReRiff.ClientConfig.dll').write_bytes(b'existing helper')
        (self.game/'ReRiff-Launcher.exe').write_bytes(b'old launcher')
        (self.game/'server.ini').write_bytes(b'external settings\n')
        (self.game/'localserver/save.json').write_bytes(b'saved scores')
        (self.game/'localserver/install-state.json').write_text(json.dumps({'version': 'old', 'backup_dir': str(self.root/'backups')}))
        self.work = self.root/'work'
        self.work.mkdir()

    def package(self, platform, bad_member=None, launcher=b'new launcher'):
        files = {name: (ROOT/'packaging'/name).read_bytes() for name in ['installer.py', 'shared_patch.py']}
        files['payload/ReRiff-Launcher.exe'] = launcher
        if platform == 'windows':
            files['python/python.exe'] = b'fixture runtime'
        manifest = {'platform': platform, 'version': '9.0.0', 'update_only': True, 'required_existing': {},
                    'patches': {}, 'files': {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}}
        files['manifest.json'] = json.dumps(manifest).encode()
        if bad_member:
            files[bad_member] = b'bad'
        path = self.root/('release.zip' if platform == 'windows' else 'release.run')
        if platform == 'windows':
            with zipfile.ZipFile(path, 'w') as archive:
                for name, data in files.items():
                    archive.writestr('ReRiff-9.0.0/'+name, data)
        else:
            stream = io.BytesIO()
            with tarfile.open(fileobj=stream, mode='w:gz') as archive:
                for name, data in files.items():
                    info = tarfile.TarInfo(name)
                    info.size = len(data)
                    archive.addfile(info, io.BytesIO(data))
            path.write_bytes(b'#!/bin/sh\n__RERIFF_PAYLOAD__\n'+stream.getvalue())
        return path, hashlib.sha256(path.read_bytes()).hexdigest()

    def test_linux_preflight_then_apply_preserves_settings_and_history(self):
        archive, digest = self.package('linux')
        update.prepare(archive, digest, self.work, self.game, 'linux')
        self.assertEqual((self.game/'ReRiff-Launcher.exe').read_bytes(), b'old launcher')
        update.apply(self.work)
        self.assertEqual((self.game/'ReRiff-Launcher.exe').read_bytes(), b'new launcher')
        self.assertEqual((self.game/'server.ini').read_bytes(), b'external settings\n')
        self.assertEqual((self.game/'localserver/save.json').read_bytes(), b'saved scores')
        self.assertTrue((self.work/'success').is_file())
        self.assertTrue(list((self.root/'backups/versions').iterdir()))

    def test_windows_package_preflight_and_apply(self):
        archive, digest = self.package('windows')
        update.prepare(archive, digest, self.work, self.game, 'windows')
        update.apply(self.work)
        self.assertEqual((self.game/'localserver/windows-python/python.exe').read_bytes(), b'fixture runtime')

    def test_linux_confirms_owned_server_shutdown_before_install(self):
        (self.game/'localserver/server-control.py').write_text("def stop(root):\n    raise RuntimeError('server still running')\n")
        archive, digest = self.package('linux')
        update.prepare(archive, digest, self.work, self.game, 'linux')
        with self.assertRaisesRegex(RuntimeError, 'still running'):
            update.apply(self.work)
        self.assertEqual((self.game/'ReRiff-Launcher.exe').read_bytes(), b'old launcher')
        self.assertFalse((self.work/'success').exists())

    def test_channel_mismatch_leaves_game_untouched(self):
        archive, digest = self.package('linux')
        with self.assertRaisesRegex(ValueError, 'channel mismatch'):
            update.prepare(archive, digest, self.work, self.game, 'linux', 'experimental')
        self.assertFalse((self.work/'prepared.json').exists())
        self.assertEqual((self.game/'ReRiff-Launcher.exe').read_bytes(), b'old launcher')

    def test_checksum_failure_leaves_game_untouched(self):
        archive, digest = self.package('linux')
        with self.assertRaisesRegex(ValueError, 'checksum'):
            update.prepare(archive, '0'*64, self.work, self.game, 'linux')
        self.assertFalse((self.work/'prepared.json').exists())
        self.assertEqual((self.game/'ReRiff-Launcher.exe').read_bytes(), b'old launcher')

    def test_traversal_is_rejected_on_both_platforms(self):
        for platform in ('linux', 'windows'):
            with self.subTest(platform=platform):
                archive, digest = self.package(platform, '../../../escaped')
                work = self.root/platform
                work.mkdir()
                with self.assertRaisesRegex(ValueError, 'Unsafe'):
                    update.prepare(archive, digest, work, self.game, platform)
                self.assertFalse((work/'prepared.json').exists())

    def test_apply_rolls_back_if_a_write_fails(self):
        archive, digest = self.package('linux')
        update.prepare(archive, digest, self.work, self.game, 'linux')
        original_loader = update.load_installer
        def loader(root):
            module = original_loader(root)
            replace = module.replace
            def failing(path, data):
                if path.name == 'start-server.sh':
                    raise OSError('Simulated write failure')
                replace(path, data)
            module.replace = failing
            return module
        update.load_installer = loader
        try:
            with self.assertRaises(OSError):
                update.apply(self.work)
        finally:
            update.load_installer = original_loader
        self.assertEqual((self.game/'ReRiff-Launcher.exe').read_bytes(), b'old launcher')
        self.assertFalse((self.work/'success').exists())


if __name__ == '__main__':
    unittest.main()
