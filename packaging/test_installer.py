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
        if json.loads((self.bundle/'manifest.json').read_text()).get('vanilla_originals'):
            # Keep coverage for older update-only packages as well as the new
            # source-patcher package exercised separately below.
            legacy=self.root/'legacy-update';shutil.copytree(self.bundle,legacy);self.bundle=legacy
            manifest=json.loads((legacy/'manifest.json').read_text());manifest.pop('vanilla_originals')
            (legacy/'manifest.json').write_text(json.dumps(manifest))
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


class VanillaInstallerTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.bundle=self.root/'bundle';self.game=self.root/'game'
        self.game.mkdir();self.bundle.mkdir();self.backups=self.root/'backups'
        (self.game/'Game.exe').write_bytes(b'game')
        self.originals={'ParseOctane.dll':b'vanilla endpoint', 'Parse.NetFx45.dll':b'vanilla SDK'}
        self.patched={'ParseOctane.dll':b'INI endpoint', 'Parse.NetFx45.dll':b'offline gate'}
        for name,data in self.originals.items():(self.game/name).write_bytes(data)
        files={'payload/ReRiff.ClientConfig.dll':b'helper','payload/ReRiff.Notify.dll':b'toast',
               'patcher/ReRiff-Patcher.exe':b'source-built tool','python/python.exe':b'python'}
        for name,data in files.items():
            path=self.bundle/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        manifest={'version':'test','update_only':True,'patches':{},
                  'vanilla_originals':{n:digest(b) for n,b in self.originals.items()},
                  'required_existing':{n:digest(b) for n,b in self.patched.items()},
                  'files':{n:digest(b) for n,b in files.items()}}
        (self.bundle/'manifest.json').write_text(json.dumps(manifest))
        def run(command,**kwargs):
            source,helper,output=map(Path,command[-3:])
            Path(output).write_bytes(self.patched[source.name])
            return subprocess.CompletedProcess(command,0,'','')
        self.runner=patch.object(installer.subprocess,'run',side_effect=run).start()
        self.addCleanup(patch.stopall)
        patch.object(installer.shutil,'which',return_value='/test/mono').start()

    def install(self,**kwargs):
        return installer.install(self.bundle,self.game,self.backups,'windows',**kwargs)

    def test_fresh_repeat_restore_and_keep_matching_backup(self):
        saved=self.backups/'originals/ParseOctane.dll';saved.parent.mkdir(parents=True)
        saved.write_bytes(self.originals[saved.name]);stamp=saved.stat().st_mtime_ns
        (self.game/'server.ini').write_bytes(b'existing settings')
        self.install();self.install()
        self.assertEqual(self.runner.call_count,2)
        self.assertEqual(saved.stat().st_mtime_ns,stamp)
        for name,data in self.patched.items():self.assertEqual((self.game/name).read_bytes(),data)
        self.assertEqual((self.game/'server.ini').read_bytes(),b'existing settings')
        self.install(restore=True)
        for name,data in self.originals.items():self.assertEqual((self.game/name).read_bytes(),data)

    def test_conflicting_backup_aborts_before_any_write(self):
        saved=self.backups/'originals/Parse.NetFx45.dll';saved.parent.mkdir(parents=True)
        saved.write_bytes(b'damaged backup')
        with self.assertRaisesRegex(ValueError,'backup hash mismatch'):self.install()
        self.assertEqual(saved.read_bytes(),b'damaged backup')
        self.assertFalse((saved.parent/'ParseOctane.dll').exists())
        self.runner.assert_not_called()
        for name,data in self.originals.items():self.assertEqual((self.game/name).read_bytes(),data)

    def test_unknown_game_dll_aborts_before_patching(self):
        (self.game/'Parse.NetFx45.dll').write_bytes(b'unrelated mod')
        with self.assertRaisesRegex(ValueError,'Unsupported game DLL'):self.install()
        self.runner.assert_not_called();self.assertFalse(self.backups.exists())

    def test_bad_patcher_output_aborts_without_writes(self):
        self.patched['ParseOctane.dll']=b'incorrect output'
        with self.assertRaisesRegex(ValueError,'output hash mismatch'):self.install()
        self.assertFalse(self.backups.exists())
        for name,data in self.originals.items():self.assertEqual((self.game/name).read_bytes(),data)

    def test_dry_run_does_not_create_backups_or_change_game(self):
        before={p.name:p.read_bytes() for p in self.game.iterdir()}
        self.install(dry_run=True)
        self.assertEqual(before,{p.name:p.read_bytes() for p in self.game.iterdir()})
        self.assertFalse(self.backups.exists())

    def test_write_failure_rolls_back_both_dlls(self):
        replace=installer.replace
        def fail(path,data):
            if path==self.game/'ReRiff.Notify.dll':raise OSError('write failed')
            replace(path,data)
        with patch.object(installer,'replace',side_effect=fail):
            with self.assertRaises(OSError):self.install()
        for name,data in self.originals.items():
            self.assertEqual((self.game/name).read_bytes(),data)
            self.assertEqual((self.backups/'originals'/name).read_bytes(),data)
        self.assertFalse((self.game/'ReRiff.ClientConfig.dll').exists())


if __name__=='__main__':unittest.main()
