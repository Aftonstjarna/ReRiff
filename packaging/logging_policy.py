"""Shared client logging preference; reading never creates directories."""
import configparser
from pathlib import Path

def client_settings(game):
    ini=configparser.ConfigParser(interpolation=None)
    for name in ('server.ini','client.ini'):
        try:
            raw=(Path(game)/name).read_bytes()
            ini.read_string(raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'))
        except FileNotFoundError:pass
    return ini

def client_config(game):
    path=Path(game)/'client.ini'
    if path.is_file():return path.read_bytes()
    import io
    source=client_settings(game);result=configparser.ConfigParser(interpolation=None)
    for section in ('graphics','languages','launcher','diagnostics'):
        if source.has_section(section):result[section]=dict(source[section])
    output=io.StringIO();result.write(output);return output.getvalue().encode('utf-8')

def logging_enabled(game, default=None, category=None):
    if category is not None:
        if not logging_enabled(game, default):return False
        try:
            ini=client_settings(game)
            group={'client':'connections','network':'connections','news':'connections','engine_menu':'game','singleplayer':'game','unicode':'text','metadata':'text'}.get(category,category)
            key='log_'+(group if ini.has_option('diagnostics','log_'+group) else category)
            return ini.getboolean('diagnostics',key,fallback=True)
        except (OSError,ValueError,configparser.Error):return True
    game=Path(game)
    try:
        ini=client_settings(game)
        if ini.has_option('diagnostics','logging_enabled'):return ini.getboolean('diagnostics','logging_enabled')
    except (OSError,ValueError,configparser.Error):pass
    if default is not None:return default
    channel=configparser.ConfigParser(interpolation=None)
    try:
        channel.read(game/'localserver/client-channel.ini',encoding='utf-8-sig')
        if channel.has_option('client','channel'):return channel.get('client','channel')=='experimental'
        return '-' in (game/'localserver/client-version.txt').read_text()
    except (OSError,ValueError,configparser.Error):return False

def logging_config(raw,enabled):
    encoding='utf-16-le' if raw.startswith(b'\xff\xfe') else 'utf-16-be' if raw.startswith(b'\xfe\xff') else 'utf-8'
    bom=raw[:2] if encoding.startswith('utf-16') else b'\xef\xbb\xbf' if raw.startswith(b'\xef\xbb\xbf') else b''
    text=raw[len(bom):].decode(encoding);newline='\r\n' if '\r\n' in text else '\n'
    lines=text.replace('\r\n','\n').splitlines();section=False;found=False;at=None
    for i,rawline in enumerate(lines):
        line=rawline.strip()
        if line.startswith('['):
            section=line.lower()=='[diagnostics]'
            if section:at=i
        if section and '=' in line and line.split('=',1)[0].strip().lower()=='logging_enabled':
            lines[i]='logging_enabled = '+str(enabled).lower();found=True
    if not found:
        value='logging_enabled = '+str(enabled).lower()
        if at is None:lines+=['','[diagnostics]',value]
        else:lines.insert(at+1,value)
    return bom+(newline.join(lines)+newline).encode(encoding)

def archive_grouped_logs(game):
    """Retain superseded component logs byte-for-byte outside the active list."""
    import uuid
    logs=Path(game)/'logs'
    names={'ReRiff-'+name+'.log' for name in ('client','network','news','engine-menu','singleplayer','unicode','metadata')}
    sources=[p for p in logs.iterdir() if p.is_file() and (p.name in names or any(p.name.startswith('legacy-') and p.name.endswith('-'+name) for name in names))] if logs.is_dir() else []
    if not sources:return {}
    archive=logs/'archive'/('before-grouped-'+uuid.uuid4().hex)
    archive.mkdir(parents=True)
    moved={}
    for source in sources:
        target=archive/source.name
        source.rename(target)
        moved[str(source)]=str(target)
    return moved

# UTC and milliseconds match native/managed diagnostics. Wrap only human logs,
# never machine-readable stdout, INI completion records or JSONL traces.
def timestamp_text(text):
    from datetime import datetime, timezone
    import re
    stamp = '['+datetime.now(timezone.utc).isoformat(timespec='milliseconds').replace('+00:00', 'Z')+'] '
    return ''.join((line if re.match(r'^\[\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{3}Z\] ', line) else stamp+line)+'\n'
                   for line in str(text).splitlines())


def diagnostic_header(game, create_client_id=False):
    import os, platform, uuid
    def clean(value):
        return ''.join(' ' if ord(c)<32 or 127<=ord(c)<160 or c in '[]\u2028\u2029' else c for c in str(value))[:256] or 'unavailable'
    values={};version='unavailable';client='unavailable'
    if game is not None:
        game=Path(game)
        try:
            path=game/'localserver/diagnostic-client-id.txt'
            if create_client_id:
                path.parent.mkdir(parents=True,exist_ok=True)
                try:
                    with path.open('x',encoding='ascii') as output:output.write(str(uuid.uuid4()))
                except FileExistsError:pass
            if path.stat().st_size==36:client=str(uuid.UUID(path.read_text(encoding='ascii')))
        except (OSError,ValueError,UnicodeError):pass
        try:
            path=game/'localserver/diagnostic-context.ini'
            if path.stat().st_size<=8192:
                raw=path.read_bytes();ini=configparser.ConfigParser(interpolation=None)
                ini.read_string(raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'))
                values=dict(ini['diagnostic']) if ini.has_section('diagnostic') else {}
        except (OSError,UnicodeError,configparser.Error):pass
        try:
            path=game/'localserver/client-version.txt'
            if path.stat().st_size<256:version=clean(path.read_text().strip())
        except (OSError,UnicodeError):pass
    def identity(key):
        try:return str(uuid.UUID(values.get(key,'')))
        except ValueError:return 'unavailable'
    return ('[diagnostic] client_id='+client+'; player='+clean(values.get('player','unavailable'))+
            '; account_id='+identity('account_id')+'; server_id='+identity('server_id')+
            '; identity=last-known; installed_client='+version+'; platform='+platform.system()+
            '; process=python; pid='+str(os.getpid()))


class TimestampWriter:
    """Line-aware diagnostic sink; preserves print fragments and tracebacks."""
    def __init__(self, stream, game=None):
        import threading
        self.stream = stream
        if game is None:
            name=getattr(stream,'name',None)
            if isinstance(name,str) and Path(name).parent.name=='logs':game=Path(name).parent.parent
        self.game=game
        self.header_pending=game is not None
        self.pending = ''
        self.continuation = False
        self.lock = threading.RLock()

    def _emit(self, newline):
        if self.header_pending:
            self.stream.write(timestamp_text(diagnostic_header(self.game,create_client_id=True)))
            self.header_pending=False
        text = self.pending.rstrip('\r')
        self.pending = ''
        stamped = text if self.continuation else timestamp_text(text or '\n').rstrip('\n')
        self.stream.write(stamped+('\n' if newline else ''))
        if newline:
            self.stream.flush()
        self.continuation = not newline

    def write(self, text):
        with self.lock:
            for part in text.splitlines(keepends=True):
                self.pending += part.rstrip('\n')
                if part.endswith('\n'):
                    self._emit(True)
                elif len(self.pending) >= 65536:
                    self._emit(False)
        return len(text)

    def flush(self):
        with self.lock:
            if self.pending:
                self._emit(False)
            self.stream.flush()

    def __getattr__(self, name):
        return getattr(self.stream, name)
