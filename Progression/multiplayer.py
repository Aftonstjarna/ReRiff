"""Opt-in trusted-client progression. Accounts are allocated on first connection; credentials stay client-owned.

The native save stays opaque. Gameplay observations are NOT anticheat proof.
SQLite commits the save and server-derived progression counters together.
"""
import argparse
import base64
from contextlib import contextmanager
import hashlib
import hmac
import json
import os
from pathlib import Path
import re
import secrets
import sqlite3
import threading
import time
import uuid

MAX_SAVE = 1024 * 1024
MAX_XP = 2000000000


class Refused(ValueError):
    def __init__(self, message, status=400):
        super().__init__(message)
        self.status = status


def whole(value, maximum, minimum=0):
    if type(value) is not int or not minimum <= value <= maximum:
        raise Refused('Invalid integer')
    return value


def exact(value, names):
    if not isinstance(value, dict) or set(value) != set(names.split()):
        raise Refused('Missing or unknown fields')


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


def snapshot(value):
    exact(value, 'cycleXp coins achievements equippedCar equippedPack ownedCars ownedPacks')
    whole(value['cycleXp'], MAX_XP)
    whole(value['coins'], 2147483647)
    whole(value['equippedCar'], 21)
    whole(value['equippedPack'], 8)
    for key, maximum in [('achievements', 38), ('ownedCars', 21), ('ownedPacks', 197)]:
        items = value[key]
        if not isinstance(items, list) or len(items) > maximum + 1:
            raise Refused('Invalid ownership or achievements')
        for item in items:
            whole(item, maximum)
        if items != sorted(set(items)):
            raise Refused('Lists must contain sorted unique IDs')
    return value


class Store:
    def __init__(self, data):
        directory = Path(data)/'progression'
        directory.mkdir(parents=True, exist_ok=True)
        self.path = directory/'profiles.sqlite3'
        with self.connect() as db:
            db.execute('CREATE TABLE IF NOT EXISTS accounts (account_id TEXT PRIMARY KEY, token_hash TEXT UNIQUE NOT NULL, checkpoint TEXT, operation TEXT, request_hash TEXT)')

    @contextmanager
    def connect(self):
        db = sqlite3.connect(self.path, timeout=10)
        try:
            db.row_factory = sqlite3.Row
            db.execute('PRAGMA synchronous=FULL')
            with db:
                yield db
        finally:
            db.close()

    def provision(self, token=None):
        token = secrets.token_hex(32) if token is None else token
        digest = self.token_hash(token)
        with self.connect() as db:
            try:
                db.execute('INSERT INTO accounts(account_id, token_hash) VALUES (?, ?)', (str(uuid.uuid4()), digest))
            except sqlite3.IntegrityError:
                raise Refused('Account already provisioned; credentials were not replaced', 409) from None
        return token

    def connect_account(self, token, create):
        """Retrying first connection with the same secret returns the same UUID."""
        digest = self.token_hash(token)
        with self.connect() as db:
            db.execute('BEGIN IMMEDIATE')
            row = db.execute('SELECT account_id FROM accounts WHERE token_hash=?', (digest,)).fetchone()
            if row is not None:
                return row['account_id']
            if not create:
                raise Refused('Account credential is no longer valid; restore your credential backup', 401)
            account = str(uuid.uuid4())
            db.execute('INSERT INTO accounts(account_id, token_hash) VALUES (?, ?)', (account, digest))
            return account

    @staticmethod
    def token_hash(token):
        if not isinstance(token, str) or not re.fullmatch('[0-9a-f]{64}', token):
            raise Refused('Invalid progression credential', 401)
        return hashlib.sha256(token.encode('ascii')).hexdigest()

    def account(self, token):
        digest = self.token_hash(token)
        with self.connect() as db:
            row = db.execute('SELECT account_id FROM accounts WHERE token_hash=?', (digest,)).fetchone()
        if row is None:
            raise Refused('Invalid progression credential', 401)
        return row['account_id']

    def rotate_token(self, account):
        token = secrets.token_hex(32)
        with self.connect() as db:
            if db.execute('UPDATE accounts SET token_hash=? WHERE account_id=?', (self.token_hash(token), account)).rowcount != 1:
                raise Refused('Unknown progression account', 404)
        return token

    def identity(self, account):
        with self.connect() as db:
            row = db.execute('SELECT account_id FROM accounts WHERE account_id=?', (account,)).fetchone()
        if row is None:
            raise Refused('Unknown progression account', 401)
        return {'accountId': row['account_id']}

    def read(self, account):
        with self.connect() as db:
            row = db.execute('SELECT checkpoint FROM accounts WHERE account_id=?', (account,)).fetchone()
        if row is None:
            raise Refused('Unknown progression account', 401)
        return json.loads(row['checkpoint']) if row['checkpoint'] else None

    def commit(self, account, request, policy, prestige=False, importing=False):
        exact(request, 'operationId expectedGeneration balanceRevision nativeSave snapshot')
        try:
            if str(uuid.UUID(request['operationId'])) != request['operationId']:
                raise ValueError()
        except (ValueError, TypeError, AttributeError):
            raise Refused('Invalid operation ID') from None
        whole(request['expectedGeneration'], 9223372036854775806)
        if request['balanceRevision'] != policy.revision:
            raise Refused('Balance changed; reconnect before saving', 409)
        encoded = request['nativeSave']
        if not isinstance(encoded, str) or len(encoded) > (MAX_SAVE+2)//3*4:
            raise Refused('Native save is too large', 413)
        try:
            native = base64.b64decode(encoded, validate=True)
        except (ValueError, TypeError):
            raise Refused('Invalid native save encoding') from None
        if not 1 <= len(native) <= MAX_SAVE:
            raise Refused('Invalid native save size', 413)
        observed = snapshot(request['snapshot'])
        digest = hashlib.sha256((('import' if importing else str(prestige))+canonical(request)).encode()).hexdigest()
        with self.connect() as db:
            db.execute('BEGIN IMMEDIATE')
            row = db.execute('SELECT * FROM accounts WHERE account_id=?', (account,)).fetchone()
            if row is None:
                raise Refused('Unknown progression account', 401)
            if row['operation'] == request['operationId']:
                if not hmac.compare_digest(row['request_hash'], digest):
                    raise Refused('Operation ID reused with different content', 409)
                return json.loads(row['checkpoint'])
            previous = json.loads(row['checkpoint']) if row['checkpoint'] else None
            generation = previous['generation'] if previous else 0
            if request['expectedGeneration'] != generation:
                raise Refused('Progress changed on another session; reconnect', 409)
            if importing:
                if previous is not None or prestige:
                    raise Refused('Import requires an empty server account', 409)
                main, count = observed['cycleXp'], 0
            elif previous is None:
                if prestige or observed['cycleXp'] != 0 or observed['achievements']:
                    raise Refused('New server profiles must start with zero XP and no achievements')
                main, count = 0, 0
            elif prestige:
                old = previous['snapshot']
                if old['equippedCar'] not in old['ownedCars'] or old['equippedCar']*9+old['equippedPack'] not in old['ownedPacks']:
                    raise Refused('Equipped car and paint must be owned before prestige')
                if not policy.prestige_eligible(old['achievements'], previous['prestigeCount']):
                    raise Refused('Prestige is not eligible', 409)
                if observed['cycleXp'] != 0 or observed['coins'] != 0 or observed['achievements'] != [18]:
                    raise Refused('Prestige must reset cycle XP, coins and achievements')
                if any(observed[key] != old[key] for key in ('equippedCar', 'equippedPack')):
                    raise Refused('Prestige must retain equipped car and paint')
                if observed['ownedCars'] != [old['equippedCar']] or observed['ownedPacks'] != [old['equippedCar']*9+old['equippedPack']]:
                    raise Refused('Prestige must retain only equipped ownership')
                main, count = previous['mainXp'], previous['prestigeCount']+1
                whole(count, 2147483647)
            else:
                delta = observed['cycleXp'] - previous['cycleXp']
                if delta < 0:
                    raise Refused('Cycle XP can only decrease through confirmed prestige', 409)
                if not set(previous['snapshot']['achievements']).issubset(observed['achievements']):
                    raise Refused('Achievements can only clear through prestige', 409)
                main, count = previous['mainXp']+delta, previous['prestigeCount']
                whole(main, 18446744073709551615)
            result = dict(generation=generation+1, mainXp=main, cycleXp=observed['cycleXp'], prestigeCount=count,
                          balanceRevision=policy.revision, nativeSave=encoded, snapshot=observed,
                          nativeSaveHash=hashlib.sha256(native).hexdigest())
            db.execute('UPDATE accounts SET checkpoint=?, operation=?, request_hash=? WHERE account_id=?',
                       (canonical(result), request['operationId'], digest, account))
            return result


def register(app, data, server_id, balance_module):
    from flask import jsonify, request
    mode = os.environ.get('RERIFF_PROGRESSION_MODE', 'disabled')
    environ = dict(os.environ)
    store = Store(data) if mode == 'trusted' else None
    password = environ.get('RERIFF_SERVER_PASSWORD', '')
    password_digest = hashlib.sha256(password.encode('utf-8')).digest()
    admission_lock = threading.Lock()
    admission_times = []

    def check_password():
        if not password:
            return
        encoded = request.headers.get('X-ReRiff-Server-Password', '')
        try:
            if len(encoded) > 4096:
                raise ValueError()
            supplied = base64.b64decode(encoded, validate=True)
        except ValueError:
            supplied = b''
        if not hmac.compare_digest(hashlib.sha256(supplied).digest(), password_digest):
            raise Refused('Server password required or incorrect', 403)

    def limit_admission():
        # Bound anonymous registration and password attempts, without trusting
        # proxy-forwarded IP headers or retaining an unbounded per-IP map.
        with admission_lock:
            now = time.monotonic()
            admission_times[:] = [stamp for stamp in admission_times if now-stamp < 60]
            if len(admission_times) >= 600:
                raise Refused('Too many connection attempts; try again shortly', 429)
            admission_times.append(now)

    def bound_request():
        if request.path.startswith('/reriff/progression'):
            request.max_content_length = 4096 if request.path.endswith('/connect') else 2*MAX_SAVE

    app.before_request_funcs.setdefault(None, []).insert(0, bound_request)

    def policy():
        if mode not in ('disabled', 'trusted'):
            raise Refused('Invalid progression mode', 503)
        if mode != 'trusted':
            raise Refused('Server progression is disabled', 404)
        try:
            result = balance_module.Balance.from_env(environ)
        except balance_module.PolicyError:
            raise Refused('Invalid server balance policy', 503) from None
        config = result.config
        # Existing native client hooks support these settings. Do not advertise
        # enforcement or silently ignore policy the client cannot implement.
        if config['anti_cheat'] != 'disabled' or config['xp'] != {'main_rate': '1', 'prestige_rate': '1'} or config['unlocks']['car_levels'] or config['prestige']['required_achievements'] != 'native_all':
            raise Refused('Trusted progression requires anticheat disabled, native XP rates, car requirements and achievements', 503)
        return result

    def capability():
        try:
            policy()
            return {'serverCheckpoints': True, 'prestigeReset': True, 'trustedClientProgression': True, 'initialSaveImport': True}
        except Refused:
            return {'serverCheckpoints': False, 'prestigeReset': False, 'trustedClientProgression': False}

    def info():
        try:
            policy()
            response = jsonify(protocol=1, serverId=server_id, automaticAccounts=True,
                               passwordRequired=bool(password), capabilities=capability())
        except Refused as error:
            response = jsonify(error=str(error)); response.status_code = error.status
        response.headers['Cache-Control'] = 'no-store'
        return response

    def connect_account():
        try:
            policy()
            limit_admission()
            check_password()
            header = request.headers.get('Authorization', '')
            token = header[7:] if header.startswith('Bearer ') else ''
            raw = request.get_data(cache=True)
            if len(raw) > 4096:
                raise Refused('Connection request too large', 413)
            try:
                payload = json.loads(raw, object_pairs_hook=balance_module.unique_object)
            except (ValueError, RecursionError):
                raise Refused('Invalid connection request') from None
            exact(payload, 'create')
            if type(payload['create']) is not bool:
                raise Refused('Invalid account creation flag')
            account = store.connect_account(token, payload['create'])
            response = jsonify(protocol=1, serverId=server_id, accountId=account)
        except Refused as error:
            response = jsonify(error=str(error)); response.status_code = error.status
        response.headers['Cache-Control'] = 'no-store'
        return response

    def handle(prestige=False, importing=False):
        try:
            current = policy()
            check_password()
            header = request.headers.get('Authorization', '')
            account = store.account(header[7:] if header.startswith('Bearer ') else '')
            if request.method == 'GET':
                checkpoint = store.read(account)
            else:
                # Bound reads even if the request uses chunked transfer.
                raw = request.get_data(cache=True)
                if len(raw) > 2*MAX_SAVE:
                    raise Refused('Progression request is too large', 413)
                try:
                    payload = json.loads(raw, object_pairs_hook=balance_module.unique_object)
                except (ValueError, RecursionError):
                    raise Refused('Invalid progression JSON') from None
                checkpoint = store.commit(account, payload, current, prestige, importing)
            response = jsonify(protocol=1, serverId=server_id, **store.identity(account),
                               balance=current.config, balanceRevision=current.revision, checkpoint=checkpoint,
                               capabilities=capability())
            response.headers['Cache-Control'] = 'no-store'
            return response
        except Refused as error:
            response = jsonify(error=str(error))
            response.status_code = error.status
            response.headers['Cache-Control'] = 'no-store'
            return response

    app.add_url_rule('/reriff/progression/info', 'progression_info', info, methods=['GET'])
    app.add_url_rule('/reriff/progression/connect', 'progression_connect', connect_account, methods=['POST'])
    app.add_url_rule('/reriff/progression', 'progression_load', handle, methods=['GET'])
    app.add_url_rule('/reriff/progression/checkpoint', 'progression_save', handle, methods=['POST'])
    app.add_url_rule('/reriff/progression/import', 'progression_import', lambda: handle(importing=True), methods=['POST'])
    app.add_url_rule('/reriff/progression/prestige', 'progression_prestige', lambda: handle(True), methods=['POST'])
    return capability


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['create-account', 'rotate-token'])
    parser.add_argument('--data', default=os.environ.get('RERIFF_DATA_DIR', '/data'))
    parser.add_argument('--account-id', help='Immutable server account UUID for credential rotation')
    args = parser.parse_args()
    try:
        store = Store(args.data)
        if args.command == 'create-account':
            if args.account_id is not None:
                raise Refused('create-account allocates a new immutable account UUID; omit --account-id')
            token = store.provision()
        else:
            if args.account_id is None:
                raise Refused('rotate-token requires --account-id')
            token = store.rotate_token(args.account_id)
    except Refused as error:
        parser.exit(1, str(error)+'\n')
    account_id = Store(args.data).account(token)
    print('multiplayer_account_id = '+account_id)
    print('multiplayer_token = '+token)
