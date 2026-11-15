#!/usr/bin/env python3
"""Exercise opt-in progression using disposable storage and synthetic saves."""
import argparse
import base64
import json
import subprocess
import time
import urllib.error
import urllib.request
import uuid


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine',default='docker')
    parser.add_argument('--image',default='reriff-server:local')
    parser.add_argument('--client-command',nargs='+',help='Optional compiled transport test command (for example wine test.exe)')
    args=parser.parse_args()
    name='reriff-progression-'+uuid.uuid4().hex[:10]
    volume=name+'-data'
    token='a'*64  # Synthetic credential, only in this isolated test volume.
    opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))
    def engine(*command):return subprocess.check_output([args.engine,*command],text=True).strip()
    def request(path,body=None,credential=token):
        headers={'Content-Type':'application/json'}
        if credential:headers['Authorization']='Bearer '+credential
        data=json.dumps(body).encode() if isinstance(body,dict) else body
        req=urllib.request.Request(base+path,data=data,headers=headers)
        with opener.open(req,timeout=10) as response:return json.loads(response.read())
    def ready():
        deadline=time.monotonic()+20
        while time.monotonic()<deadline:
            try:return request('/')['serverId']
            except (OSError,ValueError):time.sleep(.1)
        raise RuntimeError('Test server did not start')
    def start():
        engine('run','-d','--name',name,'--read-only','--tmpfs','/tmp','--cap-drop=all',
               '--security-opt=no-new-privileges','-p','127.0.0.1::5144','-v',volume+':/data',
               '-e','RERIFF_PROGRESSION_MODE=trusted',args.image)
        return 'http://127.0.0.1:'+engine('port',name,'5144/tcp').splitlines()[0].rsplit(':',1)[1]
    engine('volume','create',volume)
    try:
        base=start();server_id=ready()
        account=engine('exec',name,'python','-c',"import sys;sys.path.insert(0,'/app/Progression');from multiplayer import Store;s=Store('/data');t=s.provision('a'*64);print(s.account(t))")
        assert str(uuid.UUID(account))==account
        if args.client_command:
            subprocess.run([*args.client_command,base,server_id,account],check=True)
        else:
            state=request('/reriff/progression');assert state['checkpoint'] is None
            revision=state['balanceRevision']
            def commit(generation,xp,achievements,prestige=False):
                observed=dict(cycleXp=xp,coins=0,achievements=achievements,equippedCar=5,equippedPack=2,ownedCars=[5],ownedPacks=[47])
                payload=dict(operationId=str(uuid.uuid4()),expectedGeneration=generation,balanceRevision=revision,
                             nativeSave=base64.b64encode(('synthetic-save-'+str(generation)).encode()).decode(),snapshot=observed)
                path='/reriff/progression/'+('prestige' if prestige else 'checkpoint')
                result=request(path,payload)
                assert request(path,payload)==result  # Lost-ack replay.
                return result
            commit(0,0,[])
            commit(1,12000,[i for i in range(39) if i!=18])
            commit(2,0,[18],True)
        before=request('/reriff/progression')
        assert before['accountId']==account and before['serverId']==server_id
        progress=before['checkpoint']
        assert (progress['prestigeCount'],progress['mainXp'],progress['cycleXp'])==(1,12000,0)
        for credential in (None,'b'*64):
            try:request('/reriff/progression',credential=credential)
            except urllib.error.HTTPError as error:assert error.code==401
            else:raise AssertionError('Unauthenticated progression accepted')
        try:request('/reriff/progression/checkpoint',b'x'*(2097152+1))
        except urllib.error.HTTPError as error:assert error.code==413
        else:raise AssertionError('Oversized progression accepted')
        assert request('/classes/Progression')['results']==[]
        trace=engine('exec',name,'python','-c',"from pathlib import Path;print(''.join(p.read_text() for p in Path('/data/traces').glob('*.jsonl')))")
        assert token not in trace and progress['nativeSave'] not in trace
        assert 'private_progression' in trace
        engine('stop','--time','10',name);engine('rm',name)
        base=start();assert ready()==server_id
        assert request('/reriff/progression')==before
        print('PASS: trusted progression HTTP, credentials, immutable UUID, confirmed reset/retry, private traces, bounded bodies and complete checkpoint after container recreation')
    except Exception:
        subprocess.run([args.engine,'logs',name],check=False)
        raise
    finally:
        subprocess.run([args.engine,'rm','-f',name],stdout=subprocess.DEVNULL,check=False)
        subprocess.run([args.engine,'volume','rm',volume],stdout=subprocess.DEVNULL,check=False)


if __name__=='__main__':main()
