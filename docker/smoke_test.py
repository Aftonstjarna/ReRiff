#!/usr/bin/env python3
"""Exercise the real image on an isolated port/volume; leave the live server alone."""
import argparse
import json
from pathlib import Path
import subprocess
import time
import urllib.error
import urllib.request
import uuid


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine',default='docker')
    parser.add_argument('--image',default='reriff-server:local')
    args=parser.parse_args()
    name='reriff-smoke-'+uuid.uuid4().hex[:10]
    volume=name+'-data'

    def engine(*command):
        return subprocess.check_output([args.engine,*command],text=True).strip()

    opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))

    def request(path,method='GET',body=None,mime='application/json'):
        if isinstance(body,dict):body=json.dumps(body).encode()
        req=urllib.request.Request(base+path,data=body,method=method,headers={'Content-Type':mime})
        with opener.open(req,timeout=5) as response:
            data=response.read()
            return response.status,data

    def ready():
        deadline=time.monotonic()+20
        while time.monotonic()<deadline:
            try:
                if json.loads(request('/')[1]).get('server')=='RiffApiLocal':return
            except (OSError,ValueError,urllib.error.URLError):pass
            time.sleep(.2)
        raise RuntimeError('Container did not become ready')

    engine('volume','create',volume)
    try:
        engine('run','-d','--name',name,'--read-only','--tmpfs','/tmp','--cap-drop=all',
               '--security-opt=no-new-privileges','-p','127.0.0.1::5144','-v',volume+':/data',args.image)
        port=engine('port',name,'5144/tcp').splitlines()[0].rsplit(':',1)[1]
        base='http://127.0.0.1:'+port
        ready()
        assert engine('exec',name,'id','-u')=='10001'
        assert request('/admin')[0]==200
        _,data=request('/classes/Score','POST',{'Score':12345,'SongID':'container-smoke'})
        oid=json.loads(data)['objectId']
        binary=b'\x00\xffghost-smoke'*128
        status,data=request('/files/container.ghost','POST',binary,'application/octet-stream')
        assert status==201
        assert json.loads(data)['url']==base+'/files/container.ghost'
        assert request('/files/container.ghost')[1]==binary
        # Two clients have cached the same source under different server IDs.
        request('/classes/Score','POST',{'Score':54321,'SongID':'second-cache','Owner':'second'})
        manifest={'songs':[{'song_id':song,'sha256':'a'*64,'title':'Shared','artist':'Test'}
                           for song in ('container-smoke','second-cache')]}
        assert json.loads(request('/reriff/songs/register','POST',manifest)[1])['registered']==2
        def shared_scores():
            for song in ('container-smoke','second-cache'):
                where=urllib.parse.urlencode({'where':json.dumps({'SongID':song}),'order':'-Score'})
                scores=json.loads(request('/classes/Score?'+where)[1])['results']
                assert [row['Score'] for row in scores]==[54321,12345]
        shared_scores()
        request('/login?username=container-test&password=TRACE_SECRET')
        engine('exec',name,'python','/app/docker/healthcheck.py')
        engine('restart',name)
        # Engines may reassign an ephemeral host port on restart.
        port=engine('port',name,'5144/tcp').splitlines()[0].rsplit(':',1)[1]
        base='http://127.0.0.1:'+port
        ready()
        assert json.loads(request('/classes/Score/'+oid)[1])['Score']==12345
        shared_scores()
        assert request('/files/container.ghost')[1]==binary
        trace=engine('exec',name,'python','-c',"from pathlib import Path; print(''.join(p.read_text() for p in Path('/data/traces').glob('*.jsonl')))")
        assert 'TRACE_SECRET' not in trace
        assert '[redacted]' in trace
        assert 'container.ghost' in trace
        public_test="""import importlib.util,tempfile
from pathlib import Path
spec=importlib.util.spec_from_file_location('container_server','/app/docker/server.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as tmp:
 app=module.create_app(Path(tmp),'https://riff.example.test')
 response=app.test_client().post('/files/public.ghost',data=b'ghost',content_type='application/octet-stream')
 assert response.json['url']=='https://riff.example.test/files/public.ghost'
"""
        engine('exec',name,'python','-c',public_test)
        print('PASS: non-root/read-only image, admin page, healthcheck, scores and ghosts across restart, credential redaction, public file URL')
    except Exception:
        subprocess.run([args.engine,'logs',name],check=False)
        raise
    finally:
        subprocess.run([args.engine,'rm','-f',name],stdout=subprocess.DEVNULL,check=False)
        subprocess.run([args.engine,'volume','rm',volume],stdout=subprocess.DEVNULL,check=False)


if __name__=='__main__':main()
