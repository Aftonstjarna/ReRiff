"""Shared client logging preference; reading never creates directories."""
import configparser
from pathlib import Path

def logging_enabled(game, default=None):
    game=Path(game);ini=configparser.ConfigParser(interpolation=None)
    try:
        raw=(game/'server.ini').read_bytes()
        ini.read_string(raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'))
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
