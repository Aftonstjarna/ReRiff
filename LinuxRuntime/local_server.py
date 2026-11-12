"""Per-installation native server lifecycle, invoked by the bundled runtime."""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import secrets
import signal
import subprocess
import tempfile
import time
import urllib.error
import urllib.request

from host_process import file_lock, stamp, windows_identity


def atomic(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=path.name+'.', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as stream:
            stream.write(data)
        os.replace(name, path)
    finally:
        Path(name).unlink(missing_ok=True)



class LocalServer:
    def __init__(self, root):
        self.root = Path(root).resolve()
        self.control = self.root/'native'
        self.config_path = self.control/'config.json'
        self.record_path = self.control/'process.json'
        self.windows = os.name == 'nt'
        self.image = self.root/'windows-server/entry.py' if self.windows else self.root/'ReRiff-Server-x86_64.AppImage'
        self.python = self.root/'windows-python/python.exe'
        self.logs = self.root.parent/'logs'
        self.logs.mkdir(parents=True, exist_ok=True)
        if self.config_path.exists():
            self.config = json.loads(self.config_path.read_text())
        else:
            self.config = {'schema': 1, 'port': 5144, 'data_dir': 'data'}
            legacy = self.root/'podman'
            # Reuse existing data in place. Bind the same port before opening
            # it, so a still-running old container prevents concurrent startup.
            if (legacy/'data').is_dir() and not (self.root/'data').exists():
                self.config['data_dir'] = 'podman/data'
                if (legacy/'config.json').is_file():
                    old = json.loads((legacy/'config.json').read_text())
                    self.config['port'] = old.get('port', 5144)
        data_dir = self.config.get('data_dir', 'data')
        if data_dir not in ('data', 'podman/data'):
            raise ValueError('Local server data_dir must be data or podman/data.')
        self.data = self.root/data_dir
        port = self.config.get('port')
        if self.config.get('schema') != 1 or type(port) is not int or not 1024 <= port <= 65535:
            raise ValueError('Invalid localserver/native/config.json port; use 1024–65535.')
        self.address = 'http://127.0.0.1:'+str(port)
        self.owner = hashlib.sha256(os.fsencode(self.root)).hexdigest()

    @contextmanager
    def lock(self):
        self.control.mkdir(parents=True, exist_ok=True)
        with file_lock(self.control/'control.lock'):
            yield

    def record(self):
        try:
            return json.loads(self.record_path.read_text())
        except FileNotFoundError:
            return None

    def owned(self, record):
        if not isinstance(record, dict) or record.get('owner') != self.owner:
            return False
        pid = record.get('pid')
        if type(pid) is not int or pid <= 1 or stamp(pid) != record.get('start'):
            return False
        if self.windows:
            identity = windows_identity(pid)
            return bool(identity and identity[0] == record.get('start') and isinstance(record.get('token'), str) and len(record['token']) == 64 and os.path.normcase(os.path.realpath(identity[1])) == os.path.normcase(os.path.realpath(self.python)))
        try:
            if Path(f'/proc/{pid}').stat().st_uid != os.getuid():
                return False
            env = Path(f'/proc/{pid}/environ').read_bytes().split(b'\0')
            token = record.get('token', '')
            return len(token) == 64 and ('RERIFF_LOCAL_TOKEN='+token).encode() in env
        except OSError:
            return False

    def ready(self, record):
        if not self.owned(record):
            return False
        try:
            opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
            with opener.open(self.address+'/reriff-local/status', timeout=.5) as response:
                return json.load(response).get('token') == record['token']
        except (OSError, ValueError):
            return False

    def status(self):
        record = self.record()
        if self.ready(record):
            return 'running'
        if self.owned(record):
            raise RuntimeError('The owned server is starting or unresponsive. See logs/ReRiff-localserver.log.')
        return 'stopped' if self.image.is_file() and self.config_path.is_file() else 'uninstalled'

    def install(self, asset, expected):
        if not asset or not expected or len(expected) != 64:
            raise ValueError('A verified server download and SHA-256 are required.')
        source = Path(asset)
        if hashlib.sha256(source.read_bytes()).hexdigest() != expected.lower():
            raise ValueError('Local server download checksum mismatch.')
        if self.owned(self.record()):
            raise RuntimeError('Stop the local server before replacing its application.')
        if self.windows:
            from windows_package import install
            return install(self, source)
        self.root.mkdir(parents=True, exist_ok=True)
        fd, name = tempfile.mkstemp(prefix='server-', suffix='.AppImage', dir=self.root)
        try:
            with os.fdopen(fd, 'wb') as stream:
                stream.write(source.read_bytes())
            os.chmod(name, 0o755)
            result = subprocess.run([name, '--self-test'], capture_output=True, text=True, timeout=60)
            if result.returncode:
                raise RuntimeError('Downloaded server failed its self-test: '+result.stderr[-1000:])
            os.replace(name, self.image)
            atomic(self.config_path, json.dumps(self.config, indent=2)+'\n')
        finally:
            Path(name).unlink(missing_ok=True)
        return 'Local server installed. Use '+self.address+' in Secondary server.'

    def start(self):
        state = self.status()
        if state == 'running':
            return 'Local server is already running at '+self.address
        if state == 'uninstalled':
            raise RuntimeError('Install the optional local server first.')
        token = secrets.token_hex(32)
        env = dict(os.environ, RERIFF_LOCAL_TOKEN=token)
        # Leave the game's Proton/Steam paths out of the native server loader.
        for key in ('LD_PRELOAD', 'LD_LIBRARY_PATH', 'PYTHONHOME', 'PYTHONPATH'):
            env.pop(key, None)
        with (self.logs/'ReRiff-localserver.log').open('ab') as log:
            command = ([str(self.python), '-I', str(self.image)] if self.windows else [str(self.image)]) + ['--serve', '--root', str(self.root)]
            options = {'creationflags': subprocess.CREATE_NO_WINDOW} if self.windows else {'start_new_session': True}
            child = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=log, env=env, **options)
        deadline = time.monotonic()+30
        while time.monotonic() < deadline:
            record = self.record()
            if record and record.get('token') == token and self.ready(record):
                return 'Local server running at '+self.address
            if child.poll() is not None:
                raise RuntimeError('Local server exited during startup. See logs/ReRiff-localserver.log.')
            time.sleep(.1)
        raise RuntimeError('Local server is not ready. See logs/ReRiff-localserver.log; no process was force-stopped.')

    def stop(self):
        record = self.record()
        if not self.owned(record):
            return 'No server owned by this installation is running.'
        if self.windows:
            if not self.ready(record):
                raise RuntimeError('The owned server is unresponsive; no process was force-stopped. See logs/ReRiff-localserver.log.')
            request = urllib.request.Request(self.address+'/reriff-local/stop', data=b'', headers={'X-ReRiff-Local-Token': record['token']}, method='POST')
            opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
            with opener.open(request, timeout=5) as response:
                response.read()
        # pidfd ties the signal to this exact process, avoiding PID reuse.
        elif hasattr(os, 'pidfd_open') and hasattr(signal, 'pidfd_send_signal'):
            fd = os.pidfd_open(record['pid'])
            try:
                if self.owned(record):
                    signal.pidfd_send_signal(fd, signal.SIGTERM)
            finally:
                os.close(fd)
        elif self.owned(record):
            os.kill(record['pid'], signal.SIGTERM)
        deadline = time.monotonic()+15
        while time.monotonic() < deadline:
            if not self.owned(record):
                return 'Local server stopped.'
            time.sleep(.1)
        raise RuntimeError('The local server is still stopping; no force kill was attempted.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('status', 'install', 'start', 'stop'))
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--asset', type=Path)
    parser.add_argument('--sha256')
    parser.add_argument('--result', type=Path, required=True)
    args = parser.parse_args()
    outcome = {'ok': 'false', 'state': 'uninstalled', 'address': 'http://127.0.0.1:5144'}
    server = None
    try:
        server = LocalServer(args.root)
        outcome['address'] = server.address
        with server.lock():
            if args.action == 'install':
                message = server.install(args.asset, args.sha256)
            elif args.action == 'start':
                message = server.start()
            elif args.action == 'stop':
                message = server.stop()
            else:
                message = 'Local server: '+server.status()
            outcome.update(ok='true', state=server.status(), message=message)
    except Exception as error:
        if server is not None:
            try:
                # Keep Stop available while an owned process is still starting.
                if server.owned(server.record()):
                    outcome['state'] = 'running'
                elif server.image.is_file() and server.config_path.is_file():
                    outcome['state'] = 'stopped'
            except (OSError, ValueError):
                pass
        outcome['message'] = str(error).replace('\n', ' ')
        try:
            logs = args.root.resolve().parent/'logs'
            logs.mkdir(parents=True, exist_ok=True)
            with (logs/'ReRiff-localserver.log').open('a', encoding='utf-8') as log:
                log.write(type(error).__name__+': '+str(error)+'\n')
        except OSError:
            pass
    # Windows GetPrivateProfileString reads Unicode INI reliably with a BOM.
    text = '[result]\n'+''.join(k+'='+str(v)+'\n' for k, v in outcome.items())
    args.result.parent.mkdir(parents=True, exist_ok=True)
    temporary = args.result.with_name(args.result.name+'.partial')
    temporary.write_text(text, encoding='utf-16')
    temporary.replace(args.result)
    return 0 if outcome['ok'] == 'true' else 1


if __name__ == '__main__':
    raise SystemExit(main())
