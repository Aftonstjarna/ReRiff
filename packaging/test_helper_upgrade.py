"""Exercise signed-helper upgrades without distributing game DLL fixtures."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import installer
from shared_patch import digest


class HelperUpgradeTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.bundle=self.root/'bundle';self.game=self.root/'game'
        self.game.mkdir();self.bundle.mkdir();self.backups=self.root/'backups'
        (self.game/'Game.exe').write_bytes(b'fixture')
        self.names=('ParseOctane.dll','Parse.NetFx45.dll')
        self.old={n:b'old-'+n.encode() for n in self.names}
        self.original={n:b'original-'+n.encode() for n in self.names}
        self.fresh={n:b'fresh-signed-'+n.encode() for n in self.names}
        self.upgraded={n:b'upgraded-signed-'+n.encode() for n in self.names}
        files={'payload/ReRiff.ClientConfig.dll':b'signed helper',
               'payload/ReRiff.NewsHeaders.dll':b'signed headers','python/python.exe':b'python'}
        for n,data in files.items():
            p=self.bundle/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        self.manifest={'version':'1.2.1-2','patches':{},'update_only':True,
            'files':{n:digest(b) for n,b in files.items()},
            'vanilla_originals':{n:digest(b) for n,b in self.original.items()},
            'required_existing':{n:digest(b) for n,b in self.fresh.items()},
            'client_upgrades':{n:{digest(self.old[n]):digest(self.upgraded[n])} for n in self.names}}
        (self.bundle/'manifest.json').write_text(json.dumps(self.manifest))
        for n,data in self.old.items():(self.game/n).write_bytes(data)
        (self.game/'server.ini').write_bytes(b'keep settings')
        def run(command,**kwargs):
            source,helper,output=map(Path,command[-3:])
            if '--retarget-helpers' in command: data=self.upgraded[source.name]
            elif '--offline-gate' in command:data=b'intermediate'
            else:data=self.fresh['Parse.NetFx45.dll' if '--news-header' in command else source.name]
            output.write_bytes(data)
            return subprocess.CompletedProcess(command,0,'','')
        self.runner=patch.object(installer.subprocess,'run',side_effect=run)
        self.mock_run=self.runner.start();self.addCleanup(self.runner.stop)
        self.mono=patch.object(installer.shutil,'which',return_value='/test/mono')
        self.mono.start();self.addCleanup(self.mono.stop)

    def install(self):return installer.install(self.bundle,self.game,self.backups,'windows')

    def test_missing_backups_upgrade_and_repeat(self):
        self.install();self.install()
        self.assertEqual(self.mock_run.call_count,2)
        state=json.loads((self.game/'localserver/install-state.json').read_text())
        for n,data in self.upgraded.items():
            self.assertEqual((self.game/n).read_bytes(),data)
            self.assertEqual(state['patched'][n],digest(data))
        self.assertFalse((self.backups/'originals').exists())
        self.assertEqual((self.game/'server.ini').read_bytes(),b'keep settings')

    def test_verified_backups_rebuild_fresh_and_remain_unchanged(self):
        stamps={}
        for n,data in self.original.items():
            p=self.backups/'originals'/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
            stamps[n]=p.stat().st_mtime_ns
        self.install()
        for n,data in self.fresh.items():
            self.assertEqual((self.game/n).read_bytes(),data)
            self.assertEqual((self.backups/'originals'/n).stat().st_mtime_ns,stamps[n])

    def test_bad_output_preserves_old_install(self):
        self.upgraded['ParseOctane.dll']=b'bad output'
        with self.assertRaisesRegex(ValueError,'output hash mismatch'):self.install()
        for n,data in self.old.items():self.assertEqual((self.game/n).read_bytes(),data)
        self.assertFalse(self.backups.exists())

    def test_backup_conflict_preserves_backup_and_game(self):
        p=self.backups/'originals/Parse.NetFx45.dll';p.parent.mkdir(parents=True);p.write_bytes(b'conflict')
        with self.assertRaisesRegex(ValueError,'backup hash mismatch'):self.install()
        self.assertEqual(p.read_bytes(),b'conflict');self.mock_run.assert_not_called()
        for n,data in self.old.items():self.assertEqual((self.game/n).read_bytes(),data)


if __name__=='__main__':unittest.main()
