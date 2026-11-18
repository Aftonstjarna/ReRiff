#!/usr/bin/env python3
"""Run the compiled client against the real progression protocol on localhost."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import uuid
from unittest.mock import patch
from flask import Flask
from werkzeug.serving import make_server

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Progression'))
import balance
import multiplayer


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client-command',nargs='+',required=True)
    args=parser.parse_args()
    with tempfile.TemporaryDirectory() as data:
        store=multiplayer.Store(data);token=store.provision('a'*64);account=store.account(token)
        server_id=str(uuid.uuid4());app=Flask(__name__)
        environment={key:value for key,value in os.environ.items() if not key.startswith(('RERIFF_BALANCE_','RERIFF_PROGRESSION_'))}
        environment['RERIFF_PROGRESSION_MODE']='trusted'
        with patch.dict(os.environ,environment,clear=True):
            multiplayer.register(app,data,server_id,balance)
        for password in ('', 'test-password-only'):
            current=Flask(__name__)
            with patch.dict(os.environ,dict(environment,RERIFF_SERVER_PASSWORD=password),clear=True):
                multiplayer.register(current,data,server_id,balance)
            server=make_server('127.0.0.1',0,current,threaded=True)
            thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
            try:
                command=[*args.client_command,'http://127.0.0.1:'+str(server.server_port),server_id,account]
                if password:command.append(password)
                subprocess.run(command,check=True)
            finally:
                server.shutdown();thread.join(timeout=5);server.server_close()
        checkpoint=multiplayer.Store(data).read(account)
        assert checkpoint['prestigeCount']==1 and checkpoint['mainXp']==12000
        print('PASS: compiled client and server protocol share immutable account identity and committed prestige')


if __name__=='__main__':main()
