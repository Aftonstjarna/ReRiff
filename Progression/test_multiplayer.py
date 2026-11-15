import base64
import concurrent.futures
import copy
import importlib.util
import json
import os
import sqlite3
from pathlib import Path
import tempfile
import unittest
import uuid
from unittest.mock import patch
from flask import Flask
import balance
from multiplayer import Store, Refused, register


class MultiplayerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.store = Store(self.temp.name)
        self.token = self.store.provision()
        self.other = self.store.provision()
        self.account = self.store.account(self.token)
        self.other_account = self.store.account(self.other)
        self.policy = balance.Balance.from_env({})

    def request(self, generation=0, xp=0, complete=False):
        return dict(operationId=str(uuid.uuid4()), expectedGeneration=generation,
                    balanceRevision=self.policy.revision, nativeSave=base64.b64encode(b'native-save-'+str(xp).encode()).decode(),
                    snapshot=dict(cycleXp=xp, coins=120, achievements=[i for i in range(39) if i != 18] if complete else [],
                                  equippedCar=5, equippedPack=2, ownedCars=[5], ownedPacks=[47]))

    def eligible(self):
        self.store.commit(self.account, self.request(), self.policy)
        return self.store.commit(self.account, self.request(1, 12000, True), self.policy)

    def reset_request(self, generation=2):
        request = self.request(generation)
        request['snapshot'].update(coins=0, achievements=[18])
        request['nativeSave'] = base64.b64encode(b'original-native-reset-save').decode()
        return request

    def test_fresh_native_default_can_precede_unlock_processing(self):
        request=self.request();request['snapshot'].update(ownedCars=[],ownedPacks=[])
        result=self.store.commit(self.account,request,self.policy)
        self.assertEqual(result['mainXp'],0)
        self.assertEqual(result['prestigeCount'],0)
        with self.assertRaises(Refused):self.store.commit(self.account,self.reset_request(1),self.policy,True)

    def test_failed_database_write_keeps_whole_previous_checkpoint(self):
        previous=self.eligible()
        with self.store.connect() as db:
            db.execute("CREATE TRIGGER fail_save BEFORE UPDATE OF checkpoint ON accounts BEGIN SELECT RAISE(ABORT, 'test storage failure'); END")
        with self.assertRaises(sqlite3.IntegrityError):self.store.commit(self.account,self.reset_request(),self.policy,True)
        self.assertEqual(Store(self.temp.name).read(self.account),previous)

    def test_prestige_atomic_state_and_restart(self):
        old = self.eligible()
        request = self.reset_request()
        result = self.store.commit(self.account, request, self.policy, True)
        self.assertEqual((result['mainXp'],result['cycleXp'],result['prestigeCount'],result['generation']), (12000,0,1,3))
        self.assertEqual(result['nativeSave'], request['nativeSave'])
        self.assertEqual(Store(self.temp.name).read(self.account),result)
        self.assertIsNone(self.store.read(self.other_account))
        race=self.request(3,50);race['snapshot']['achievements']=[18]
        self.assertEqual(self.store.commit(self.account,race,self.policy)['mainXp'],12050)
        self.assertEqual(old['prestigeCount'],0)

    def test_retry_is_idempotent_and_conflicting_reuse_refused(self):
        self.eligible();request=self.reset_request()
        result=self.store.commit(self.account,request,self.policy,True)
        self.assertEqual(self.store.commit(self.account,request,self.policy,True),result)
        changed=copy.deepcopy(request);changed['nativeSave']=base64.b64encode(b'changed').decode()
        with self.assertRaises(Refused):self.store.commit(self.account,changed,self.policy,True)
        with self.assertRaises(Refused):self.store.commit(self.account,request,self.policy,False)
        self.assertEqual(self.store.read(self.account),result)

    def test_concurrent_resets_advance_once(self):
        self.eligible()
        def run(_):
            try:return self.store.commit(self.account,self.reset_request(),self.policy,True)
            except Refused:return None
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            results=list(pool.map(run,range(4)))
        self.assertEqual(sum(r is not None for r in results),1)
        self.assertEqual(self.store.read(self.account)['prestigeCount'],1)

    def test_ineligible_and_partial_resets_leave_save_unchanged(self):
        previous=self.store.commit(self.account,self.request(),self.policy)
        with self.assertRaises(Refused):self.store.commit(self.account,self.reset_request(1),self.policy,True)
        self.assertEqual(self.store.read(self.account),previous)
        previous=self.store.commit(self.account,self.request(1,12000,True),self.policy)
        cases=[('coins',1),('cycleXp',1),('achievements',[]),('ownedCars',[0,5]),('ownedPacks',[0,47]),('equippedCar',0),('equippedPack',0)]
        for key,value in cases:
            with self.subTest(key=key):
                request=self.reset_request();request['snapshot'][key]=value
                with self.assertRaises(Refused):self.store.commit(self.account,request,self.policy,True)
                self.assertEqual(self.store.read(self.account),previous)

    def test_no_import_or_implicit_reset_or_client_counters(self):
        with self.assertRaises(Refused):self.store.commit(self.account,self.request(0,100),self.policy)
        request=self.request();request['prestigeCount']=12
        with self.assertRaises(Refused):self.store.commit(self.account,request,self.policy)
        old=self.eligible()
        with self.assertRaises(Refused):self.store.commit(self.account,self.request(2,0),self.policy)
        self.assertEqual(self.store.read(self.account),old)

    def test_policy_revision_and_prestige_cap(self):
        old=self.eligible();changed=balance.Balance.from_env({'RERIFF_BALANCE_MAXIMUM_PRESTIGE_COUNT':'0'})
        request=self.reset_request()
        with self.assertRaises(Refused):self.store.commit(self.account,request,changed,True)
        request['balanceRevision']=changed.revision
        with self.assertRaises(Refused):self.store.commit(self.account,request,changed,True)
        self.assertEqual(self.store.read(self.account),old)

    def test_credentials_are_separate_from_legacy_parse_and_not_plaintext(self):
        self.assertEqual(self.store.account(self.token),self.account)
        self.assertEqual(str(uuid.UUID(self.account)),self.account)
        self.assertNotEqual(self.account,self.other_account)
        self.assertEqual(Store(self.temp.name).identity(self.account), {"accountId":self.account})
        self.assertEqual(self.store.account(self.other),self.other_account)
        for token in ['', 'r:parse-token', '0'*64]:
            with self.assertRaises(Refused):self.store.account(token)
        with self.assertRaises(Refused):self.store.provision(self.token)
        self.assertNotIn(self.token.encode(),self.store.path.read_bytes())

    def test_immutable_account_survives_credential_rotation(self):
        saved=self.eligible()
        new_token=self.store.rotate_token(self.account)
        with self.assertRaises(Refused):self.store.account(self.token)
        self.assertEqual(self.store.account(new_token),self.account)
        self.assertEqual(self.store.read(self.account),saved)
        request=self.request(2,13000,True);request['username']='changed-name'
        with self.assertRaises(Refused):self.store.commit(self.account,request,self.policy)
        self.assertEqual(self.store.read(self.account),saved)

    def app(self, **settings):
        app=Flask(__name__)
        with patch.dict(os.environ,settings,clear=True):
            flags=register(app,self.temp.name,'12345678-1234-1234-1234-123456789abc',balance)
        balance.register_endpoint(app,self.temp.name,'12345678-1234-1234-1234-123456789abc',flags)
        return app.test_client()

    def test_http_auth_opt_in_capabilities_and_private_storage(self):
        headers={'Authorization':'Bearer '+self.token}
        disabled=self.app()
        self.assertEqual(disabled.get('/reriff/progression',headers=headers).status_code,404)
        client=self.app(RERIFF_PROGRESSION_MODE='trusted')
        self.assertEqual(client.get('/reriff/progression').status_code,401)
        self.assertEqual(client.get('/reriff/progression',headers={'X-Parse-Session-Token':self.token}).status_code,401)
        response=client.get('/reriff/progression',headers=headers)
        self.assertEqual(response.json['accountId'],self.account)
        self.assertNotIn('steamAccountId',response.json)
        self.assertEqual(response.headers['Cache-Control'],'no-store')
        self.assertIsNone(response.json['checkpoint'])
        flags=client.get('/reriff/balance').json['capabilities']
        self.assertTrue(flags['prestigeReset']);self.assertFalse(flags['progressionEnforced']);self.assertFalse(flags['antiCheatVerified'])
        response=client.post('/reriff/progression/checkpoint',headers=headers,json=self.request())
        self.assertEqual(response.status_code,200)
        self.assertEqual(client.get('/reriff/progression',headers={'Authorization':'Bearer '+self.other}).json['checkpoint'],None)
        self.assertFalse((Path(self.temp.name)/'classes').exists())

    def test_http_rejects_bad_policy_and_bounded_payload(self):
        headers={'Authorization':'Bearer '+self.token}
        for settings in ({'RERIFF_PROGRESSION_MODE':'bad'}, {'RERIFF_PROGRESSION_MODE':'trusted','RERIFF_BALANCE_ANTI_CHEAT':'required'}, {'RERIFF_PROGRESSION_MODE':'trusted','RERIFF_BALANCE_MAIN_XP_RATE':'2'}):
            client=self.app(**settings)
            self.assertEqual(client.get('/reriff/progression',headers=headers).status_code,503)
        client=self.app(RERIFF_PROGRESSION_MODE='trusted')
        self.assertEqual(client.post('/reriff/progression/checkpoint',headers=headers,data=b'x'*(2097152+1)).status_code,413)
        self.assertEqual(client.post('/reriff/progression/checkpoint',headers=headers,data=b'{"snapshot":{},"snapshot":{}}').status_code,400)
        request=self.request();request['snapshot']['cycleXp']=True
        self.assertEqual(client.post('/reriff/progression/checkpoint',headers=headers,json=request).status_code,400)
        self.assertIsNone(self.store.read(self.account))

    def test_automatic_accounts_discovery_retry_and_reconnect(self):
        client=self.app(RERIFF_PROGRESSION_MODE='trusted')
        info=client.get('/reriff/progression/info')
        self.assertEqual(info.status_code,200)
        self.assertFalse(info.json['passwordRequired'])
        self.assertTrue(info.json['automaticAccounts'])
        token='c'*64;headers={'Authorization':'Bearer '+token}
        first=client.post('/reriff/progression/connect',headers=headers,json={'create':True})
        self.assertEqual(first.status_code,200)
        self.assertEqual(str(uuid.UUID(first.json['accountId'])),first.json['accountId'])
        for create in (True,False):
            self.assertEqual(client.post('/reriff/progression/connect',headers=headers,json={'create':create}).json,first.json)
        self.assertEqual(client.get('/reriff/progression',headers=headers).json['accountId'],first.json['accountId'])
        self.assertEqual(client.post('/reriff/progression/connect',headers={'Authorization':'Bearer '+'d'*64},json={'create':False}).status_code,401)
        self.store.rotate_token(first.json['accountId'])
        self.assertEqual(client.post('/reriff/progression/connect',headers=headers,json={'create':False}).status_code,401)
        self.assertEqual(self.app().get('/reriff/progression/info').status_code,404)

    def test_automatic_account_concurrent_retry(self):
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            ids=list(pool.map(lambda _:self.store.connect_account('e'*64,True),range(8)))
        self.assertEqual(len(set(ids)),1)

    def test_server_password_checks_creation_and_existing_progression(self):
        password='test-pässword'
        client=self.app(RERIFF_PROGRESSION_MODE='trusted',RERIFF_SERVER_PASSWORD=password)
        self.assertTrue(client.get('/reriff/progression/info').json['passwordRequired'])
        headers={'Authorization':'Bearer '+'f'*64}
        for supplied in ('','wrong','%%invalid%%'):
            headers['X-ReRiff-Server-Password']=supplied
            self.assertEqual(client.post('/reriff/progression/connect',headers=headers,json={'create':True}).status_code,403)
        with self.assertRaises(Refused):self.store.account('f'*64)
        headers['X-ReRiff-Server-Password']=base64.b64encode(password.encode()).decode()
        joined=client.post('/reriff/progression/connect',headers=headers,json={'create':True})
        self.assertEqual(joined.status_code,200)
        self.assertEqual(client.get('/reriff/progression',headers=headers).status_code,200)
        del headers['X-ReRiff-Server-Password']
        self.assertEqual(client.get('/reriff/progression',headers=headers).status_code,403)
        self.assertEqual(client.post('/reriff/progression/checkpoint',headers=headers,json=self.request()).status_code,403)
        self.assertEqual(client.post('/reriff/progression/prestige',headers=headers,json=self.reset_request()).status_code,403)

    def test_admission_is_bounded_and_strict(self):
        client=self.app(RERIFF_PROGRESSION_MODE='trusted')
        headers={'Authorization':'Bearer '+'c'*64}
        self.assertEqual(client.post('/reriff/progression/connect',headers=headers,json={'create':'true'}).status_code,400)
        self.assertEqual(client.post('/reriff/progression/connect',headers=headers,data=b'x'*4097).status_code,413)
        for _ in range(598):
            self.assertEqual(client.post('/reriff/progression/connect',headers=headers,json={'create':True}).status_code,200)
        self.assertEqual(client.post('/reriff/progression/connect',headers=headers,json={'create':True}).status_code,429)
