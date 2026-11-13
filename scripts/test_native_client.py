"""Black-box checks for the Python-free Linux installer and update host.

Python is a build/test dependency only. Every child runs with Python and command
line tools absent from PATH. Optional original-file checks use private fixtures.
"""
import argparse
import hashlib
import importlib.util
import sys
import gzip
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch

parser=argparse.ArgumentParser()
parser.add_argument('--helper',type=Path,required=True)
parser.add_argument('--bundle',type=Path)
parser.add_argument('--original-dir',type=Path)
parser.add_argument('--installer',type=Path)
args=parser.parse_args()
HELPER=args.helper.resolve()
ROOT=Path(__file__).resolve().parents[1]
def digest(data): return hashlib.sha256(data).hexdigest()

class NativeClientTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='native client ü ');self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.game=self.root/'game with spaces';self.game.mkdir();self.bundle=self.root/'bundle';self.bundle.mkdir()
        (self.game/'Game.exe').write_bytes(b'unchanged original executable fixture')
        self.config=b'[server]\nexternal_url = https://example.invalid\n[diagnostics]\nlogging_enabled = false\n'
        (self.game/'server.ini').write_bytes(self.config)
        (self.game/'singleplayer').mkdir();(self.game/'singleplayer/scores.json').write_bytes(b'keep scores')
        self.manifest={'version':'test-native','platform':'linux','channel':'experimental','update_only':True,'native_linux':True,
                       'files':{},'patches':{},'required_existing':{},'vanilla_originals':{}}
        self.env=dict(os.environ,PATH='/no-system-tools')
        for name in ('PYTHONHOME','PYTHONPATH','LD_LIBRARY_PATH','LD_PRELOAD'):self.env.pop(name,None)
    def package(self,files=None):
        for name,data in (files or {'payload/a.txt':b'new value'}).items():
            p=self.bundle/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
            self.manifest['files'][name]=digest(data)
        (self.bundle/'manifest.json').write_text(json.dumps(self.manifest))
    def call(self,*values,success=True):
        result=subprocess.run([str(HELPER),'--client-host',*map(str,values)],env=self.env,capture_output=True,text=True,timeout=180)
        self.assertEqual(result.returncode==0,success,result.stdout+result.stderr);return result
    def install(self,*values,success=True):return self.call('install','--bundle',self.bundle,'--game-dir',self.game,*values,success=success)
    def retained(self):
        self.assertEqual((self.game/'server.ini').read_bytes(),self.config)
        self.assertEqual((self.game/'singleplayer/scores.json').read_bytes(),b'keep scores')
        self.assertEqual((self.game/'Game.exe').read_bytes(),b'unchanged original executable fixture')
    def test_install_repeat_restore_and_executable_host(self):
        original=b'original DLL fixture';ready=b'known patched DLL fixture'
        self.manifest['vanilla_originals']={'A.dll':digest(original)};self.manifest['required_existing']={'A.dll':digest(ready)}
        (self.game/'A.dll').write_bytes(ready);saved=self.game/'.bak/originals/A.dll';saved.parent.mkdir(parents=True);saved.write_bytes(original);stamp=saved.stat().st_mtime_ns
        self.package();self.install('--no-music-link');self.install('--no-music-link')
        self.assertEqual((self.game/'localserver/install-state.json').stat().st_mode & 0o077,0);self.assertTrue(os.access(self.game/'localserver/ReRiff-ClientHost',os.X_OK));self.assertEqual(saved.stat().st_mtime_ns,stamp)
        self.install('--restore');self.assertEqual((self.game/'A.dll').read_bytes(),original);self.retained()
    def test_unknown_dll_and_conflicting_backup_are_retained(self):
        self.manifest['vanilla_originals']={'A.dll':digest(b'original')};self.manifest['required_existing']={'A.dll':digest(b'ready')}
        (self.game/'A.dll').write_bytes(b'unknown');self.package();self.install(success=False);self.assertFalse((self.game/'a.txt').exists())
        (self.game/'A.dll').write_bytes(b'ready');backup=self.game/'.bak/originals/A.dll';backup.parent.mkdir(parents=True);backup.write_bytes(b'wrong backup')
        self.install(success=False);self.assertEqual(backup.read_bytes(),b'wrong backup');self.retained()
    def test_logging_defaults_override_and_existing_logs_survive_disable(self):
        self.manifest['channel']='stable'
        (self.game/'server.ini').write_text('[server]\nexternal_url = https://example.invalid\n')
        self.package({'payload/localserver/diagnostics/Collect-ReRiffStartup.ps1':b'collector'})
        self.install('--no-music-link')
        self.assertFalse((self.game/'logs').exists())
        self.call('logging-status','--root',self.game,success=False)
        self.manifest['channel']='experimental';self.package({})
        self.install('--no-music-link');self.assertFalse((self.game/'logs').exists())
        self.install('--logging','--no-music-link');self.call('logging-status','--root',self.game)
        self.assertEqual((self.game/'logs/Collect-ReRiffStartup.ps1').read_bytes(),b'collector')
        (self.game/'logs/retained.log').write_bytes(b'old diagnostic')
        self.install('--no-logging','--no-music-link')
        self.assertEqual((self.game/'logs/retained.log').read_bytes(),b'old diagnostic')
        self.call('logging-status','--root',self.game,success=False)
    def test_stable_retains_only_originals_and_config_snapshots(self):
        self.manifest['channel']='stable'
        (self.game/'extra.ini').write_bytes(b'old config')
        (self.game/'ReRiff.ClientConfig.dll').write_bytes(b'old helper')
        versions=self.game/'.bak/versions';versions.mkdir(parents=True)
        (versions/(digest(b'old binary')+'-ReRiff-Launcher.exe')).write_bytes(b'old binary')
        (versions/(digest(b'old ini')+'-server.ini')).write_bytes(b'old ini')
        self.package({'payload/extra.ini':b'new config','payload/ReRiff.ClientConfig.dll':b'new helper'})
        self.install('--no-music-link')
        self.assertFalse(versions.exists())
        self.assertEqual((self.game/'.bak/configs'/(digest(b'old config')+'-extra.ini')).read_bytes(),b'old config')
        self.assertEqual((self.game/'.bak/configs'/(digest(b'old ini')+'-server.ini')).read_bytes(),b'old ini')
        self.retained()
    def test_legacy_backups_and_shortcut_migrate_without_losing_restore(self):
        original=b'original';ready=b'ready'
        self.manifest['vanilla_originals']={'A.dll':digest(original)};self.manifest['required_existing']={'A.dll':digest(ready)}
        (self.game/'A.dll').write_bytes(ready)
        saved=self.game/'bak/originals/A.dll';saved.parent.mkdir(parents=True);saved.write_bytes(original)
        local=self.game/'localserver';local.mkdir();(local/'install-state.json').write_text(json.dumps({'version':'old','backup_dir':str(saved.parent.parent)}))
        (self.game/'ReRiff GitHub.desktop').write_bytes(b'old shortcut')
        # Exercise merging when private-runtime setup has already made .bak.
        (self.game/'.bak').mkdir();(self.game/'.bak/retained').write_bytes(b'keep')
        self.package();self.install('--dry-run','--no-music-link')
        self.assertTrue(saved.exists());self.assertTrue((self.game/'ReRiff GitHub.desktop').exists())
        self.install('--no-music-link')
        self.assertFalse((self.game/'bak').exists());self.assertFalse((self.game/'ReRiff GitHub.desktop').exists())
        self.assertEqual((self.game/'.bak/retained').read_bytes(),b'keep')
        self.assertEqual((self.game/'.bak/originals/A.dll').read_bytes(),original)
        self.install('--restore');self.assertEqual((self.game/'A.dll').read_bytes(),original);self.retained()
    def test_payload_hash_and_unsafe_paths_fail_before_writes(self):
        self.package();(self.bundle/'payload/a.txt').write_bytes(b'tampered');self.install(success=False);self.assertFalse((self.game/'a.txt').exists())
        self.manifest['files']={'../escape':'0'*64};self.package({});self.install(success=False);self.retained()
    def test_transaction_failure_rolls_back_prior_files(self):
        (self.game/'a.txt').write_bytes(b'old value');(self.game/'z.txt').mkdir()
        self.package({'payload/a.txt':b'new value','payload/z.txt':b'cannot replace directory'})
        self.install('--no-music-link',success=False);self.assertEqual((self.game/'a.txt').read_bytes(),b'old value');self.retained()
    def test_symlink_destination_cannot_escape_game(self):
        outside=self.root/'outside';outside.mkdir();(self.game/'linked').symlink_to(outside,target_is_directory=True)
        self.package({'payload/linked/escape':b'blocked'});self.install(success=False);self.assertFalse((outside/'escape').exists());self.retained()
    def test_old_client_python_runtime_is_archived(self):
        local=self.game/'localserver';local.mkdir();(local/'install-state.json').write_text('{"version":"old"}')
        (local/'ReRiff-Runtime-x86_64.AppImage').write_bytes(b'old owned runtime');self.package();self.install('--no-music-link')
        self.assertFalse((local/'ReRiff-Runtime-x86_64.AppImage').exists())
        self.assertEqual(len(list((self.game/'.bak/versions').glob('*-ReRiff-Runtime-x86_64.AppImage'))),1);self.retained()
    def test_music_link_and_existing_music_protection(self):
        prefix=self.game/'prefix/pfx/drive_c/users/steamuser';prefix.mkdir(parents=True)
        music=self.root/'music';music.mkdir();(music/'song.flac').write_bytes(b'keep music')
        self.package();self.install('--music-dir',music)
        self.assertTrue((prefix/'Music').is_symlink());self.assertEqual((prefix/'Music').resolve(),music.resolve())
        self.install('--music-dir',prefix/'Music');self.assertEqual((music/'song.flac').read_bytes(),b'keep music')
        alias=self.root/'music-alias';alias.symlink_to(music,target_is_directory=True)
        (prefix/'Music').unlink();(prefix/'Music').symlink_to(alias,target_is_directory=True);stamp=(prefix/'Music').lstat().st_mtime_ns
        self.install('--music-dir',music);self.assertEqual((prefix/'Music').lstat().st_mtime_ns,stamp)
        (prefix/'Music').unlink();(prefix/'Music').mkdir();(prefix/'Music/owned-song').write_bytes(b'keep prefix music')
        self.install('--music-dir',music);self.assertFalse((prefix/'Music').is_symlink());self.assertEqual((prefix/'Music/owned-song').read_bytes(),b'keep prefix music')
    def test_music_source_symlink_cannot_create_a_loop(self):
        prefix=self.game/'prefix/pfx/drive_c/users/steamuser';prefix.mkdir(parents=True)
        (prefix/'Music').mkdir();source=self.root/'source-link';source.symlink_to(prefix/'Music',target_is_directory=True)
        self.package();self.install('--music-dir',source)
        self.assertFalse((prefix/'Music').is_symlink())
    def test_legacy_update_adapter_calls_native_installer(self):
        self.package({'payload/localserver/ReRiff-ClientHost':HELPER.read_bytes(),'payload/a.txt':b'new value'})
        sys.path.insert(0,str(ROOT/'packaging'))
        try:
            spec=importlib.util.spec_from_file_location('native_upgrade_compat',ROOT/'packaging/installer.py')
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            with patch.dict(os.environ,self.env,clear=True):module.install(self.bundle,self.game,None,'linux',dry_run=True)
            self.assertFalse((self.game/'a.txt').exists());self.assertFalse((self.game/'.bak').exists())
            (self.bundle/'payload/a.txt').write_bytes(b'tampered')
            with patch.dict(os.environ,self.env,clear=True),self.assertRaises(ValueError):module.install(self.bundle,self.game,None,'linux',dry_run=True)
        finally:sys.path.pop(0)
    def test_no_server_image_status_needs_no_python(self):
        result=self.root/'result.ini';self.call('server-control','status','--root',self.game/'localserver','--result',result)
        self.assertIn('state=uninstalled',result.read_text(encoding='utf-16'))
    def test_server_runtime_failure_reports_completion_without_fuse(self):
        local=self.game/'localserver';local.mkdir()
        image=local/'ReRiff-Server-x86_64.AppImage'
        image.write_text('#!/bin/sh\n[ "$APPIMAGE_EXTRACT_AND_RUN" = 1 ] || exit 19\nexit 7\n');image.chmod(0o755)
        result=self.root/'result.ini'
        self.call('server-control','status','--root',local,'--result',result,success=False)
        reply=result.read_text(encoding='utf-16')
        self.assertIn('ok=false',reply);self.assertIn('exited with code 7',reply)
    def archive(self,extra=None):
        self.package({'payload/a.txt':b'updated'})
        data=io.BytesIO()
        with tarfile.open(fileobj=data,mode='w:gz') as archive:
            for path in self.bundle.rglob('*'):
                if path.is_file():archive.add(path,arcname=str(path.relative_to(self.bundle)))
            if extra is not None:archive.addfile(extra)
        path=self.root/'client-linux.run';path.write_bytes(b'#!/bin/sh\n__RERIFF_PAYLOAD__\n'+data.getvalue());return path,digest(path.read_bytes())
    def prepare(self,archive,checksum,work,success=True,*extra):
        return self.call('update','--prepare','--archive',archive,'--sha256',checksum,'--game-dir',self.game,'--work',work,'--channel','experimental',*extra,success=success)
    def test_update_prepare_apply_and_private_native_host(self):
        archive,checksum=self.archive();local=self.game/'localserver';local.mkdir();shutil.copy2(ROOT/'scripts/linux-runtime.sh',local/'linux-runtime.sh')
        work=self.root/'update';work.mkdir();self.prepare(archive,checksum,work)
        self.assertTrue(os.access(work/'ReRiff-ClientHost',os.X_OK));self.assertFalse((self.game/'a.txt').exists())
        self.call('update','--work',work);self.assertTrue((work/'success').exists());self.assertEqual((self.game/'a.txt').read_bytes(),b'updated');self.retained()
    def test_update_checksum_metadata_and_archive_paths(self):
        self.config=self.config.replace(b'false',b'true');(self.game/'server.ini').write_bytes(self.config)
        archive,checksum=self.archive();work=self.root/'bad-checksum';work.mkdir();self.prepare(archive,'0'*64,work,False);self.assertTrue((work/'failure').exists())
        work=self.root/'bad-metadata';work.mkdir();(work/'latest-release.json').write_text('{"version":"wrong","artifacts":["client-linux.run"]}');self.prepare(archive,checksum,work,False)
        member=tarfile.TarInfo('../escape');archive,checksum=self.archive(member);work=self.root/'bad-path';work.mkdir();self.prepare(archive,checksum,work,False)
        member=tarfile.TarInfo('oversize');member.size=512*1024*1024+1
        archive=self.root/'oversize.run';archive.write_bytes(b'#!/bin/sh\n__RERIFF_PAYLOAD__\n'+gzip.compress(member.tobuf()+b'\0'*1024))
        work=self.root/'oversize';work.mkdir();self.prepare(archive,digest(archive.read_bytes()),work,False)
        self.assertIn('Release is too large',(self.game/'logs/update.log').read_text())
        self.assertFalse((self.root/'escape').exists());self.retained()
    @unittest.skipUnless(args.installer and args.original_dir,'Complete .run fixture is optional')
    def test_self_extractor_without_python(self):
        for name in ('Game.exe','ParseOctane.dll','Parse.NetFx45.dll','Parse.dll','libfftw3f-3.dll'):shutil.copy2(args.original_dir/name,self.game/name)
        tools=self.root/'tools';tools.mkdir()
        for name in ('mktemp','awk','tail','tar','gzip','rm','chmod'):(tools/name).symlink_to(shutil.which(name))
        env=dict(self.env,PATH=str(tools));self.assertIsNone(shutil.which('python3',path=env['PATH']))
        result=subprocess.run(['/bin/sh',str(args.installer.resolve()),'--cli','--game-dir',str(self.game),'--no-music-link'],env=env,capture_output=True,text=True,timeout=180)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertFalse((self.game/'localserver/ReRiff-Runtime-x86_64.AppImage').exists())
        self.assertTrue(os.access(self.game/'localserver/ReRiff-ClientHost',os.X_OK))
    @unittest.skipUnless(args.bundle and args.original_dir,'Private original-game fixture is optional')
    def test_full_vanilla_install_without_python(self):
        for name in ('Game.exe','ParseOctane.dll','Parse.NetFx45.dll','Parse.dll','libfftw3f-3.dll'):shutil.copy2(args.original_dir/name,self.game/name)
        baseline=digest((self.game/'Game.exe').read_bytes())
        self.call('install','--bundle',args.bundle.resolve(),'--game-dir',self.game,'--no-music-link')
        self.assertEqual(digest((self.game/'Game.exe').read_bytes()),baseline)
        required=json.loads((ROOT/'packaging/required-client.json').read_text())
        for name,expected in required.items():self.assertEqual(digest((self.game/name).read_bytes()),expected)
        self.assertFalse((self.game/'localserver/ReRiff-Runtime-x86_64.AppImage').exists())
        self.assertFalse(list((self.game/'localserver').glob('*.py')))
        self.assertFalse((self.game/'localserver/ReRiff-Server-x86_64.AppImage').exists())
        self.assertEqual((self.game/'server.ini').read_bytes(),self.config)
        self.call('install','--bundle',args.bundle.resolve(),'--game-dir',self.game,'--restore')
        for name in ('ParseOctane.dll','Parse.NetFx45.dll'):self.assertEqual((self.game/name).read_bytes(),(args.original_dir/name).read_bytes())

if __name__=='__main__':unittest.main(argv=[__file__],verbosity=2)
