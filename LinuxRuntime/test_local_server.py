import importlib.util
import json
import os
from pathlib import Path
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).parent))
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location('reriff_local', Path(__file__).with_name('local_server.py'))
local = importlib.util.module_from_spec(spec)
spec.loader.exec_module(local)


class LifecycleTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.server = local.LocalServer(Path(self.temp.name)/'game with spaces ü/localserver')

    def test_loopback_defaults_and_uninstalled_status(self):
        self.assertEqual(self.server.address, 'http://127.0.0.1:5144')
        self.assertEqual(self.server.status(), 'uninstalled')
        with self.assertRaisesRegex(RuntimeError, 'Install'):
            self.server.start()

    def test_legacy_podman_data_and_port_are_retained_in_place(self):
        legacy = self.server.root/'podman'
        (legacy/'data/classes').mkdir(parents=True)
        saved = legacy/'data/classes/Score.json'
        saved.write_text('previous scores')
        (legacy/'config.json').write_text(json.dumps({'port': 5514}))
        current = local.LocalServer(self.server.root)
        self.assertEqual(current.data, legacy/'data')
        self.assertEqual(current.address, 'http://127.0.0.1:5514')
        self.assertEqual(saved.read_text(), 'previous scores')
        self.assertFalse((self.server.root/'data').exists())

    def test_invalid_port_is_rejected(self):
        self.server.control.mkdir(parents=True)
        for port in [True, 80, 65536, '5144']:
            self.server.config_path.write_text(json.dumps({'schema': 1, 'port': port}))
            with self.assertRaises(ValueError):
                local.LocalServer(self.server.root)

    def test_stale_or_foreign_record_never_signals(self):
        self.server.control.mkdir(parents=True)
        for record in [{'pid': os.getpid(), 'start': local.stamp(os.getpid()), 'owner': 'other'},
                       {'pid': os.getpid(), 'start': 'stale', 'owner': self.server.owner, 'token': '0'*64}]:
            self.server.record_path.write_text(json.dumps(record))
            with mock.patch.object(local.os, 'kill') as kill:
                self.assertIn('No server owned', self.server.stop())
                kill.assert_not_called()

    def test_failed_verification_preserves_server_and_data(self):
        self.server.root.mkdir(parents=True)
        self.server.image.parent.mkdir(parents=True, exist_ok=True)
        self.server.image.write_bytes(b'previous application')
        data = self.server.root/'data/classes/Score.json'
        data.parent.mkdir(parents=True)
        data.write_bytes(b'previous scores')
        asset = self.server.root/'new.AppImage';asset.write_bytes(b'new application')
        with self.assertRaisesRegex(ValueError, 'checksum'):
            self.server.install(asset, '0'*64)
        self.assertEqual(self.server.image.read_bytes(), b'previous application')
        self.assertEqual(data.read_bytes(), b'previous scores')

    def test_unhealthy_owned_server_is_not_reported_stopped(self):
        with mock.patch.object(self.server, 'record', return_value={}), \
             mock.patch.object(self.server, 'owned', return_value=True), \
             mock.patch.object(self.server, 'ready', return_value=False):
            with self.assertRaisesRegex(RuntimeError, 'unresponsive'):
                self.server.status()


if __name__ == '__main__':
    unittest.main()
