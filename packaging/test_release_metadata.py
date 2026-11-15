"""Exercise the publisher's actual schema with the native updater selector."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

from release_metadata import release_metadata

ROOT = Path(__file__).resolve().parents[1]

class ReleaseMetadataTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.parser = Path(cls.temp.name)/'release-parser'
        cls.legacy = Path(cls.temp.name)/'legacy-parser'
        subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(ROOT/'NativeLauncher/test_release_channel.c'),'-o',str(cls.parser)],check=True)
        frozen=ROOT/'NativeLauncher/fixtures/release_channel-v1.3.2.h'
        if hashlib.sha256(frozen.read_bytes()).hexdigest()!='73dd0f1694c262e6b45d651e539a8c64604e8298b35148b16a34decc0343773c':
            raise AssertionError('Frozen v1.3.2 parser was changed')
        driver=Path(cls.temp.name)/'legacy.c'
        driver.write_text('#include <ctype.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\ntypedef int BOOL;\n#define TRUE 1\n#define FALSE 0\n#include "'+str(frozen)+'"\n'+
            'int main(int argc,char **argv){if(argc!=3)return 1;FILE *f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);char *s=calloc((size_t)n+1,1);if(!s||fread(s,1,(size_t)n,f)!=(size_t)n)return 3;fclose(f);ReleasePackage p;int ok=release_package(s,!strcmp(argv[2],"linux"),&p);free(s);if(!ok)return 4;puts(p.filename);return 0;}')
        subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function',str(driver),'-o',str(cls.legacy)],check=True)
    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.version = '1.4.0-beta.3'
        self.names = [f'ReRiff-{self.version}-windows.zip',f'ReRiff-{self.version}-linux.run','ReRiff-Windows-Server.zip','ReRiff-Server-x86_64.AppImage','ReRiff-CJK.zip','ReRiff-Latin.zip',f'ReRiff-{self.version}-source.tar.gz']
        for name in self.names:(self.root/name).write_bytes(name.encode())
    def tearDown(self): self.temp.cleanup()
    def parse(self, metadata, platform, valid=True):
        path=self.root/'release.json';path.write_text(json.dumps(metadata))
        result=subprocess.run([str(self.parser),'--package',str(path),platform],capture_output=True,text=True)
        self.assertEqual(result.returncode==0,valid,result.stdout+result.stderr)
        return result.stdout.splitlines()
    def test_publisher_and_native_selector_both_platforms(self):
        metadata=release_metadata(self.root,self.version,'test-commit')
        self.assertEqual(metadata['schema'],2)
        self.assertEqual(metadata['artifacts'],sorted(self.names))
        self.assertEqual(len(metadata['packages']),7)
        for platform in ('windows','linux'):
            package=next(p for p in metadata['packages'] if p['component']=='client' and p['platform']==platform)
            self.assertEqual(self.parse(metadata,platform),[package['filename'],package['sha256'],str(package['size']),'1'])
            self.assertEqual(package['size'],(self.root/package['filename']).stat().st_size)
            self.assertEqual(package['sha256'],hashlib.sha256((self.root/package['filename']).read_bytes()).hexdigest())
    def test_legacy_metadata_still_selects_client_only(self):
        metadata=release_metadata(self.root,self.version,'commit');metadata.pop('schema');metadata.pop('packages')
        for platform,suffix in [('windows','-windows.zip'),('linux','-linux.run')]:
            result=self.parse(metadata,platform)
            self.assertEqual(result[0],f'ReRiff-{self.version}{suffix}')
            self.assertEqual(result[3],'0')
    def test_released_132_parser_accepts_new_publisher_metadata(self):
        metadata=release_metadata(self.root,self.version,'commit')
        path=self.root/'release.json';path.write_text(json.dumps(metadata))
        for platform,suffix in [('windows','-windows.zip'),('linux','-linux.run')]:
            result=subprocess.run([str(self.legacy),str(path),platform],capture_output=True,text=True,check=True)
            self.assertEqual(result.stdout.strip(),f'ReRiff-{self.version}{suffix}')

    def test_explicit_identity_does_not_depend_on_filename(self):
        metadata=release_metadata(self.root,self.version,'commit')
        client=next(p for p in metadata['packages'] if p['component']=='client' and p['platform']=='windows')
        client['filename']='opaque.payload'
        metadata['artifacts']=['wrong-windows.zip','another-windows.zip']
        self.assertEqual(self.parse(metadata,'windows')[0],'opaque.payload')
    def test_broken_typed_metadata_never_uses_legacy_fallback(self):
        original=release_metadata(self.root,self.version,'commit')
        changes=[('component','server'),('architecture','arm64'),('size',0),('size',True),('size',1.5),('size',2**64),('sha256','z'*64),('filename','../client.zip')]
        for field,value in changes:
            with self.subTest(field=field,value=value):
                metadata=copy.deepcopy(original);metadata['packages'][0][field]=value
                self.parse(metadata,'windows',False)
        metadata=copy.deepcopy(original);metadata['packages'].append(copy.deepcopy(metadata['packages'][0]));self.parse(metadata,'windows',False)
        metadata=copy.deepcopy(original);metadata['schema']=3;self.parse(metadata,'windows',False)
        metadata=copy.deepcopy(original);metadata.pop('packages');self.parse(metadata,'windows',False)
    def test_missing_artifacts_cannot_be_published(self):
        (self.root/self.names[0]).unlink()
        with self.assertRaisesRegex(ValueError,'Missing'):release_metadata(self.root,self.version,'commit')

if __name__=='__main__': unittest.main()
