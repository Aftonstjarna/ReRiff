import json
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import installer
from shared_patch import apply_patch, make_patch

ROOT=Path(__file__).resolve().parent.parent


class DeltaTests(unittest.TestCase):
    def test_roundtrip_and_wrong_original(self):
        source=random.Random(42).randbytes(4096)
        target=source[:512]+b'new data'+source[1024:]+source[:1024]
        encoded=make_patch(source,target)
        self.assertEqual(apply_patch(source,encoded),target)
        with self.assertRaises(ValueError): apply_patch(b'wrong',encoded)


class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.bundle=ROOT/'build/release/linux'
        originals=ROOT/'backups/client'
        if not self.bundle.is_dir() or not originals.is_dir():
            self.skipTest('Build a release and supply original DLLs before installer integration tests.')
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.game=self.root/'game';self.game.mkdir()
        (self.game/'Game.exe').write_bytes(b'fixture')
        self.originals={n:(originals/n).read_bytes() for n in ('ParseOctane.dll','Parse.NetFx45.dll')}
        for n,b in self.originals.items():(self.game/n).write_bytes(b)
        self.backups=self.root/'project/backups'

    def install(self, **kwargs):
        return installer.install(self.bundle,self.game,self.backups,'linux',**kwargs)

    def test_install_repeat_restore_and_preservation(self):
        ini=b'[server]\nexternal_url=\nfallback_url=\n';(self.game/'server.ini').write_bytes(ini)
        data=self.game/'localserver/server/data/keep.json';data.parent.mkdir(parents=True);data.write_text('{"keep":true}')
        self.install();self.install()
        self.assertEqual((self.game/'server.ini').read_bytes(),ini)
        self.assertEqual(data.read_text(),'{"keep":true}')
        self.assertFalse(list(self.game.glob('*.reriff*')))
        env={'PYTHONPATH':str(self.game/'localserver/python-libs'),'PYTHONNOUSERSITE':'1'}
        import os
        result=subprocess.run([sys.executable,'-c','import flask,click,markupsafe; assert markupsafe.escape("<") == "&lt;"; print(flask.Flask("test").test_client().get("/").status_code)'],env=dict(os.environ,**env),capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        self.install(restore=True)
        for n,b in self.originals.items():self.assertEqual((self.game/n).read_bytes(),b)
        self.assertTrue(data.exists())

    def test_dry_run_no_writes(self):
        before=set(self.game.rglob('*'));self.install(dry_run=True)
        self.assertEqual(set(self.game.rglob('*')),before)
        self.assertFalse(self.backups.exists())

    def test_unknown_modification_rejected(self):
        (self.game/'ParseOctane.dll').write_bytes(b'unknown')
        with self.assertRaises(ValueError):self.install()
        self.assertFalse(self.backups.exists())

    def test_corrupt_package_rejected(self):
        bundle=self.root/'corrupt';shutil.copytree(self.bundle,bundle)
        (bundle/'payload/ReRiff-Launcher.exe').write_bytes(b'corrupt')
        with self.assertRaisesRegex(ValueError,'integrity'):
            installer.install(bundle,self.game,self.backups,'linux')
        self.assertFalse(self.backups.exists())

    def test_failure_rolls_back_changed_dlls(self):
        original_replace=installer.replace
        triggered=[]
        def failing(path,data):
            if path==self.game/'ReRiff.Notify.dll' and not triggered:
                triggered.append(True);raise OSError('simulated write failure')
            original_replace(path,data)
        with patch.object(installer,'replace',side_effect=failing):
            with self.assertRaises(OSError):self.install()
        self.assertTrue(triggered)
        for n,b in self.originals.items():self.assertEqual((self.game/n).read_bytes(),b)
        self.assertFalse((self.game/'ReRiff.ClientConfig.dll').exists())


if __name__=='__main__':unittest.main()
