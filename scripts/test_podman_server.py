"""Exercise optional local-container lifecycle without changing host containers."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('podman_server',ROOT/'scripts/podman-server.py')
server=importlib.util.module_from_spec(spec);spec.loader.exec_module(server)


class FakePodman:
    def __init__(self):self.calls=[];self.image=False;self.owned=None;self.remote=False;self.image_id='sha256:'+'a'*64
    def __call__(self,command,**kwargs):
        args=command[1:]
        if args[:1]==['--remote=false']:args=args[1:]
        if args[:1]==['--connection']:args=args[2:]
        self.calls.append(args);code=0;out=''
        if args[:3]==['system','connection','list']:
            out=json.dumps([{'Default':True,'Name':'podman-machine-default','URI':'ssh://user@'+('remote.example' if self.remote else '127.0.0.1')+':1234/run/podman.sock'}])
        elif args[0]=='info':out='{}'
        elif args[:2]==['container','exists']:code=0 if self.owned else 1
        elif args[:2]==['container','inspect']:out=json.dumps([self.owned])
        elif args[0]=='pull':self.image=True
        elif args[:2]==['image','inspect']:out=json.dumps([{'Id':self.image_id}])
        elif args[:2]==['image','exists']:code=0 if self.image else 1
        elif args[0]=='create':
            label=args[args.index('--label')+1].split('=',1)
            self.owned={'Config':{'Labels':{label[0]:label[1]}},'State':{'Running':False}}
        elif args[0]=='start':self.owned['State']['Running']=True
        elif args[0]=='stop':self.owned['State']['Running']=False
        elif args[0]=='logs':out='local server diagnostics\n'
        else:raise AssertionError(command)
        return subprocess.CompletedProcess(command,code,out,'')


class Response:
    status=200
    def __enter__(self):return self
    def __exit__(self,*args):pass
    def read(self,limit):return b'{"results":[]}'


class OptionalServerTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='Riff Racer ü ');self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)/'localserver';self.root.mkdir();self.engine=FakePodman()
        self.local=server.LocalServer(self.root,runner=self.engine,windows=False)
        self.which=patch.object(server.shutil,'which',return_value='/test/podman');self.which.start();self.addCleanup(self.which.stop)

    def test_fresh_status_neither_downloads_nor_creates_config(self):
        self.assertEqual(self.local.perform('status')[0],'uninstalled')
        self.assertFalse(self.local.config_dir.exists());self.assertEqual(self.engine.calls,[])

    def test_install_then_start_stop_retains_data_and_uses_loopback_only(self):
        state,message=self.local.perform('install');self.assertEqual(state,'stopped')
        self.assertFalse(any(c[0]=='start' for c in self.engine.calls))
        create=next(c for c in self.engine.calls if c[0]=='create')
        self.assertEqual(create[create.index('--publish')+1],'127.0.0.1:5144:5144/tcp')
        self.assertEqual(create[-1],self.engine.image_id)
        self.assertIn(str(self.local.config_dir/'data'),create[create.index('--volume')+1])
        self.assertIn('http://127.0.0.1:5144',message)
        self.assertIn('Secondary server',message)
        stored=self.local.config_dir/'data/keep.json';stored.write_text('scores')
        with patch.object(server.urllib.request.OpenerDirector,'open',return_value=Response()):
            self.assertEqual(self.local.perform('start')[0],'running')
        self.assertEqual(self.local.perform('status')[0],'running')
        self.assertEqual(self.local.perform('stop')[0],'stopped')
        self.assertEqual(stored.read_text(),'scores')
        self.assertEqual((self.local.logs/'ReRiff-localserver.log').read_text(),'local server diagnostics\n')
        self.assertFalse(any(c[0] in ('rm','rmi','machine') for c in self.engine.calls))
        self.assertFalse(any('--privileged' in c for c in self.engine.calls))

    def test_foreign_container_is_never_stopped_or_replaced(self):
        self.local.config_dir.mkdir();self.local.save_config()
        self.engine.owned={'Config':{'Labels':{server.OWNER:'some-other-game'}},'State':{'Running':True}}
        for action in ('install','start','stop','status'):
            with self.assertRaisesRegex(RuntimeError,'ownership'):self.local.perform(action)
        self.assertFalse(any(c[0] in ('stop','create','pull','start') for c in self.engine.calls))

    def test_removed_container_can_be_recreated_with_existing_data(self):
        self.local.perform('install');self.engine.owned=None
        self.assertEqual(self.local.perform('status')[0],'stopped')
        with patch.object(server.urllib.request.OpenerDirector,'open',return_value=Response()):
            self.assertEqual(self.local.perform('start')[0],'running')
        self.assertEqual(len([c for c in self.engine.calls if c[0]=='create']),2)

    def test_windows_uses_only_a_local_machine_and_no_selinux_relabel(self):
        self.local.windows=True;self.engine.remote=True
        with self.assertRaisesRegex(RuntimeError,'remote servers'):self.local.perform('install')
        self.engine.remote=False;self.local.perform('install')
        create=next(c for c in self.engine.calls if c[0]=='create')
        self.assertTrue(create[create.index('--volume')+1].endswith(':/data'))
        self.assertEqual(self.local.command[-2:],['--connection','podman-machine-default'])

    def test_missing_runtime_is_actionable_and_does_not_download(self):
        with patch.object(server.shutil,'which',return_value=None):
            with self.assertRaisesRegex(RuntimeError,'Install Podman first'):self.local.perform('install')
        self.assertEqual(self.engine.calls,[])

    def test_bridge_result_is_atomic_unicode_ini(self):
        result=self.root/'result.ini';server.report(result,True,'running','http://127.0.0.1:5144','Ready\nnew line')
        text=result.read_text(encoding='utf-16')
        self.assertIn('ok=true',text);self.assertIn('message=Ready new line',text)
        self.assertEqual(list(self.root.glob('result.ini.*')),[])


if __name__=='__main__':unittest.main()
