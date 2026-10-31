import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import installer
from shared_patch import apply_patch, make_patch, digest

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
        if os.environ.get('RERIFF_SYNTHETIC_TESTS')=='1':
            originals=ROOT/'build/no-original-fixtures'
        if not self.bundle.is_dir():
            self.skipTest('Build a release before installer integration tests.')
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.game=self.root/'game';self.game.mkdir()
        (self.game/'Game.exe').write_bytes(b'fixture')
        names=('ParseOctane.dll','Parse.NetFx45.dll')
        update_only=json.loads((self.bundle/'manifest.json').read_text()).get('update_only',False)
        if update_only and os.environ.get('RERIFF_SYNTHETIC_TESTS')!='1':
            originals=ROOT/'build'
        self.originals={n:(originals/n).read_bytes() if (originals/n).exists() else (b'MZ-synthetic-original-'+n.encode()) for n in names}
        if not all((originals/n).exists() for n in names):
            bundle=self.root/'synthetic-bundle';shutil.copytree(self.bundle,bundle);self.bundle=bundle
            manifest=json.loads((bundle/'manifest.json').read_text())
            for n,b in self.originals.items():
                if update_only:
                    manifest['required_existing'][n]=digest(b)
                    continue
                patched=b+b'-patched';relative=manifest['patches'][n]['file']
                encoded=make_patch(b,patched);(bundle/relative).write_bytes(encoded)
                manifest['patches'][n].update(original=digest(b),patched=digest(patched))
                manifest['files'][relative]=digest(encoded)
            (bundle/'manifest.json').write_text(json.dumps(manifest))
        for n,b in self.originals.items():(self.game/n).write_bytes(b)
        self.previous_helper=b'existing-ReRiff-helper' if update_only else None
        if self.previous_helper is not None:
            (self.game/'ReRiff.ClientConfig.dll').write_bytes(self.previous_helper)
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
        if json.loads((self.bundle/'manifest.json').read_text()).get('update_only'):
            with self.assertRaisesRegex(ValueError,'original full installer'):
                self.install(restore=True)
        else:
            self.install(restore=True)
        for n,b in self.originals.items():self.assertEqual((self.game/n).read_bytes(),b)
        self.assertTrue(data.exists())

    def test_dry_run_no_writes(self):
        before=set(self.game.rglob('*'));self.install(dry_run=True)
        self.assertEqual(set(self.game.rglob('*')),before)
        self.assertFalse(self.backups.exists())

    def test_default_backup_location_is_game_bak(self):
        state = self.game/'localserver/install-state.json'
        state.parent.mkdir(parents=True, exist_ok=True)
        state.write_text(json.dumps({'backup_dir': str(self.root/'old-project-backups')}))
        plan = installer.install(self.bundle, self.game, None, 'linux', dry_run=True)
        next_state = json.loads(plan['localserver/install-state.json'])
        self.assertEqual(Path(next_state['backup_dir']), self.game/'bak')
        self.assertFalse((self.game/'bak').exists())

    def test_empty_python_executable_uses_native_fallback(self):
        with patch.object(installer.sys, 'executable', ''), patch.object(installer.shutil, 'which', return_value='/usr/bin/python3'):
            plan = self.install(dry_run=True)
        self.assertIn(b'exec /usr/bin/python3 ', plan['localserver/start-server.sh'])
        self.assertNotIn(b"exec '' ", plan['localserver/start-server.sh'])

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
        helper=self.game/'ReRiff.ClientConfig.dll'
        if self.previous_helper is None:self.assertFalse(helper.exists())
        else:self.assertEqual(helper.read_bytes(),self.previous_helper)


if __name__=='__main__':unittest.main()
