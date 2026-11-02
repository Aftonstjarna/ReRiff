#!/usr/bin/env python3
"""Optional, per-game Podman server; no image or backend ships with the client."""
import argparse
from contextlib import contextmanager
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import urllib.request
from urllib.parse import urlsplit

IMAGE='ghcr.io/aftonstjarna/reriff:latest'
OWNER='io.reriff.localserver.owner'


def atomic(path,data,encoding='utf-8'):
    path.parent.mkdir(parents=True,exist_ok=True)
    fd,name=tempfile.mkstemp(prefix=path.name+'.',dir=path.parent)
    try:
        with os.fdopen(fd,'w',encoding=encoding) as stream:stream.write(data)
        os.replace(name,path)
    finally:
        Path(name).unlink(missing_ok=True)


class LocalServer:
    def __init__(self,root,runner=None,windows=None):
        self.root=Path(root).resolve();self.config_dir=self.root/'podman';self.config_file=self.config_dir/'config.json'
        self.logs=self.root.parent/'logs';self.logs.mkdir(parents=True,exist_ok=True)
        identity=os.path.normcase(str(self.root))
        self.owner=hashlib.sha256(identity.encode()).hexdigest()
        self.name='reriff-local-'+self.owner[:16]
        self.runner=runner or subprocess.run;self.windows=os.name=='nt' if windows is None else windows
        self.command=[]
        self.config=json.loads(self.config_file.read_text()) if self.config_file.exists() else {'schema':1,'image':IMAGE,'port':5144}
        if self.config.get('schema')!=1 or not isinstance(self.config.get('port'),int) or not 1024<=self.config['port']<=65535:
            raise ValueError('Invalid localserver/podman/config.json; port must be 1024–65535.')
        image=self.config.get('image','')
        if not isinstance(image,str) or not image.startswith('ghcr.io/aftonstjarna/reriff:') or any(c.isspace() for c in image):
            raise ValueError('The local server image must be a ghcr.io/aftonstjarna/reriff tag.')

    @property
    def address(self):return 'http://127.0.0.1:'+str(self.config['port'])

    def log(self,text):
        with (self.logs/'ReRiff-podman.log').open('a',encoding='utf-8') as stream:
            stream.write(datetime.datetime.now(datetime.timezone.utc).isoformat()+' '+text+'\n')

    def run(self,*args,timeout=30,allowed=(0,)):
        command=self.command+list(args);self.log('Command: '+repr(command))
        result=self.runner(command,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
        if result.stdout:self.log(result.stdout.rstrip())
        if result.stderr:self.log(result.stderr.rstrip())
        if result.returncode not in allowed:
            detail=(result.stderr or result.stdout or 'exit '+str(result.returncode)).strip()
            raise RuntimeError('Podman: '+detail[-900:])
        return result

    def connect(self):
        binary=shutil.which('podman')
        if not binary and self.windows:
            for folder in (Path(os.environ.get('LOCALAPPDATA',''))/'Programs/Podman',Path(os.environ.get('PROGRAMFILES',''))/'RedHat/Podman',Path(os.environ.get('PROGRAMFILES',''))/'Podman'):
                if (folder/'podman.exe').is_file():binary=str(folder/'podman.exe');break
        if not binary:raise RuntimeError('Install Podman first: https://podman.io/docs/installation . Then retry Install local server.')
        self.command=[binary]
        if self.windows:
            connections=json.loads(self.run('system','connection','list','--format','json').stdout)
            defaults=[c for c in connections if c.get('Default')]
            if len(defaults)!=1 or urlsplit(defaults[0].get('URI','')).hostname not in ('127.0.0.1','localhost','::1'):
                raise RuntimeError('Select a local Podman machine as the default connection in Podman Desktop; remote servers are not managed here.')
            self.command += ['--connection',defaults[0]['Name']]
        else:self.command += ['--remote=false']
        try:self.run('info','--format','json',timeout=15)
        except (RuntimeError,subprocess.TimeoutExpired) as error:
            raise RuntimeError('Start your local Podman machine/runtime, then retry. '+str(error)) from error

    def container(self):
        exists=self.run('container','exists',self.name,allowed=(0,1))
        if exists.returncode:return None
        containers=json.loads(self.run('container','inspect',self.name).stdout)
        if len(containers)!=1 or containers[0].get('Config',{}).get('Labels',{}).get(OWNER)!=self.owner:
            raise RuntimeError('Container ownership does not match this game folder; it was left untouched.')
        return containers[0]

    def image_present(self):
        image=self.config.get('image_id')
        return bool(image) and self.run('image','exists',image,allowed=(0,1)).returncode==0

    def state(self):
        container=self.container()
        if container:return 'running' if container.get('State',{}).get('Running') else 'stopped'
        return 'stopped' if self.image_present() else 'uninstalled'

    def save_config(self):atomic(self.config_file,json.dumps(self.config,indent=2)+'\n')

    def create(self):
        data=self.config_dir/'data';data.mkdir(parents=True,exist_ok=True)
        env=self.config_dir/'server.env'
        if not env.exists():atomic(env,'RERIFF_PUBLIC_URL='+self.address+'\n')
        # Namespace root keeps the per-game bind directory writable across native
        # rootless Linux and Windows Podman-machine drive mappings. No host privileges.
        args=['create','--name',self.name,'--label',OWNER+'='+self.owner,'--restart=no',
              '--user','0:0','--publish','127.0.0.1:'+str(self.config['port'])+':5144/tcp',
              '--env-file',str(env),'--volume',str(data)+(':/data' if self.windows else ':/data:Z'),self.config['image_id']]
        self.run(*args,timeout=60)
        self.config['container']=self.name;self.save_config()

    def collect_logs(self):
        result=self.run('logs','--tail','200',self.name,allowed=(0,))
        atomic(self.logs/'ReRiff-localserver.log',result.stdout+result.stderr)

    def perform(self,action):
        if action=='status' and not self.config_file.exists():return 'uninstalled','Optional server is not installed. Install downloads the server image only on request.'
        self.connect()
        if action=='install':
            self.config_dir.mkdir(parents=True,exist_ok=True)
            with operation_lock(self.config_dir/'operation.lock'):
                # Always verify ownership before downloading or touching an existing name.
                existing=self.container()
                if existing:return self.state(),'Local server is already installed. Address: '+self.address
                self.run('pull',self.config['image'],timeout=600)
                image=json.loads(self.run('image','inspect',self.config['image']).stdout)
                if len(image)!=1 or not image[0].get('Id'):raise RuntimeError('Podman did not report the downloaded image identity.')
                self.config['image_id']=image[0]['Id'];self.save_config();self.create()
            return 'stopped','Server installed. Click Start local server. Enter '+self.address+' in Secondary server and enable Use servers.'
        if action=='status':return self.state(),'Local server address: '+self.address+' — enter in Secondary server and enable Use servers.'
        if not self.config_file.exists():raise RuntimeError('Install the optional local server first.')
        with operation_lock(self.config_dir/'operation.lock'):
            container=self.container()
            if action=='stop':
                if container and container.get('State',{}).get('Running'):self.run('stop','--time','15',self.name,timeout=30)
                if container:self.collect_logs()
                return 'stopped' if self.image_present() else 'uninstalled','Local server stopped. Configuration and data were retained.'
            if action!='start':raise ValueError('Unknown local server action.')
            if not container:
                if not self.image_present():raise RuntimeError('The downloaded image is missing; click Install local server again.')
                self.create()
            if not container or not container.get('State',{}).get('Running'):self.run('start',self.name,timeout=60)
            deadline=time.monotonic()+30
            while time.monotonic()<deadline:
                try:
                    with urllib.request.build_opener(urllib.request.ProxyHandler({})).open(self.address+'/classes/Version',timeout=2) as response:
                        if response.status==200 and isinstance(json.loads(response.read(65536)).get('results'),list):
                            self.collect_logs();return 'running','Local server is running. Enter '+self.address+' in Secondary server, enable Use servers, and save settings.'
                except (OSError,ValueError):time.sleep(.4)
                current=self.container()
                if not current or not current.get('State',{}).get('Running'):break
            self.collect_logs();raise RuntimeError('The local server did not become ready. See logs/ReRiff-localserver.log; the container was not force-stopped.')


@contextmanager
def operation_lock(path):
    with path.open('a+b') as stream:
        stream.seek(0);stream.write(b'0');stream.flush();stream.seek(0)
        try:
            if os.name=='nt':
                import msvcrt
                msvcrt.locking(stream.fileno(),msvcrt.LK_NBLCK,1)
            else:
                import fcntl
                fcntl.flock(stream,fcntl.LOCK_EX|fcntl.LOCK_NB)
        except OSError as error:raise RuntimeError('Another local-server operation is already running.') from error
        try:yield
        finally:
            if os.name=='nt':
                stream.seek(0);msvcrt.locking(stream.fileno(),msvcrt.LK_UNLCK,1)
            else:fcntl.flock(stream,fcntl.LOCK_UN)


def report(path,ok,state,address,message):
    message=' '.join(str(message).split())
    atomic(path,'[result]\nok='+str(ok).lower()+'\nstate='+state+'\naddress='+address+'\nmessage='+message+'\n',encoding='utf-16')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('install','start','stop','status'))
    parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parent)
    parser.add_argument('--result',type=Path,required=True)
    args=parser.parse_args();server=None
    try:
        server=LocalServer(args.root);state,message=server.perform(args.action)
        report(args.result,True,state,server.address,message)
        if server.config_dir.is_dir():report(server.config_dir/'status.ini',True,state,server.address,message)
        return 0
    except Exception as error:
        if server:server.log(type(error).__name__+': '+str(error))
        report(args.result,False,'error',server.address if server else 'http://127.0.0.1:5144',str(error))
        return 1


if __name__=='__main__':raise SystemExit(main())
