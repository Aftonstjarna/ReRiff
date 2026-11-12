"""Windows package rollback and ownership checks, also run on Microsoft Windows."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

sys.path.insert(0, str(Path(__file__).parent))
import host_process
from local_server import LocalServer
import windows_package

REQUIRED = ('entry.py','local_server.py','serve.py','host_process.py','server/riffapi-local.py',
            'server/reriff-catalog.dll','server/reriff-news.dll','python-libs/flask/__init__.py','python-libs/waitress/__init__.py')


class WindowsPackageTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.server = LocalServer(Path(self.temp.name)/'Game ü with spaces/localserver')
        self.server.image = self.server.root/'windows-server/entry.py'
        self.server.image.parent.mkdir(parents=True)
        self.server.image.write_bytes(b'previous application')
        self.server.python.parent.mkdir(parents=True)
        self.server.python.write_bytes(b'private Python placeholder')
        self.saved = self.server.data/'scores.json'
        self.saved.parent.mkdir(parents=True)
        self.saved.write_bytes(b'saved scores')
        self.archive = self.server.root/'download.zip'

    def bundle(self, extra=()):
        with zipfile.ZipFile(self.archive,'w') as archive:
            for name in REQUIRED:
                archive.writestr(name,b'new application')
            for name in extra:
                # ZipInfo normally normalizes Windows separators on Windows.
                # Keep the deliberately malformed download path in this fixture.
                info = zipfile.ZipInfo('placeholder')
                info.filename = name
                archive.writestr(info,b'unsafe')

    def test_rejects_unsafe_windows_paths_without_replacing_application_or_data(self):
        for name in ('../escape','C:/escape','server\\escape','NUL.txt','server/name.','ENTRY.PY'):
            with self.subTest(name=name):
                self.bundle((name,))
                with self.assertRaisesRegex(ValueError,'Unsafe'):
                    windows_package.install(self.server,self.archive)
                self.assertEqual(self.server.image.read_bytes(),b'previous application')
                self.assertEqual(self.saved.read_bytes(),b'saved scores')
        self.assertFalse(list(self.server.root.glob('.windows-server-*')))

    def test_failed_self_test_preserves_previous_install(self):
        self.bundle()
        with mock.patch.object(subprocess,'run',return_value=subprocess.CompletedProcess([],1,'','broken')):
            with self.assertRaisesRegex(RuntimeError,'self-test'):
                windows_package.install(self.server,self.archive)
        self.assertEqual(self.server.image.read_bytes(),b'previous application')
        self.assertEqual(self.saved.read_bytes(),b'saved scores')

    def test_success_replaces_only_application(self):
        self.bundle()
        with mock.patch.object(subprocess,'run',return_value=subprocess.CompletedProcess([],0,'','')):
            windows_package.install(self.server,self.archive)
        self.assertEqual(self.server.image.read_bytes(),b'new application')
        self.assertEqual(self.saved.read_bytes(),b'saved scores')
        self.assertTrue(self.server.config_path.is_file())
        self.assertFalse(list(self.server.root.glob('.windows-server-*')))

    def test_windows_ownership_rejects_different_python_and_creation_time(self):
        self.server.windows = True
        record = {'owner':self.server.owner,'pid':123,'start':'created','token':'0'*64}
        with mock.patch('local_server.stamp',return_value='created'), mock.patch('local_server.windows_identity',return_value=('created',str(self.server.python))):
            self.assertTrue(self.server.owned(record))
        for identity in (('reused',str(self.server.python)),('created',str(self.server.root/'other.exe')),None):
            with mock.patch('local_server.stamp',return_value='created'), mock.patch('local_server.windows_identity',return_value=identity):
                self.assertFalse(self.server.owned(record))

    @unittest.skipUnless(os.name == 'nt','Windows native process API')
    def test_native_windows_process_identity(self):
        created, executable = host_process.windows_identity(os.getpid())
        self.assertEqual(created,host_process.stamp(os.getpid()))
        self.assertEqual(os.path.normcase(os.path.realpath(executable)),os.path.normcase(os.path.realpath(sys.executable)))
        self.assertIsNone(host_process.windows_identity(0))


if __name__ == '__main__':
    unittest.main()
