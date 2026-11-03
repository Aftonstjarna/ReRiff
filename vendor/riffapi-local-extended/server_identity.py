"""Persistent store identity and explicit, one-time Parse file-origin migration."""
import hashlib
import json
import os
from pathlib import Path
import tempfile
from urllib.parse import urlsplit
import uuid


def origin(value):
    parsed = urlsplit(value)
    if (parsed.scheme not in ('http', 'https') or not parsed.hostname
            or parsed.username or parsed.password or parsed.query or parsed.fragment
            or parsed.path not in ('', '/')):
        raise ValueError('Expected an HTTP(S) origin without credentials or a path')
    # Accessing port also rejects malformed port values.
    port = parsed.port
    host = parsed.hostname.lower()
    if ':' in host:
        host = '[' + host + ']'
    suffix = '' if port is None or port == (443 if parsed.scheme == 'https' else 80) else ':' + str(port)
    return parsed.scheme.lower() + '://' + host + suffix


def atomic_write(path, data):
    path = Path(path)
    fd, temporary = tempfile.mkstemp(prefix='.reriff-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def server_id(data_dir):
    """Publish a complete UUID atomically; never replace an existing identity."""
    data = Path(data_dir)
    data.mkdir(parents=True, exist_ok=True)
    path = data / 'server-id'
    if not path.exists():
        fd, temporary = tempfile.mkstemp(prefix='.server-id-', dir=data)
        try:
            with os.fdopen(fd, 'w', encoding='ascii') as stream:
                stream.write(str(uuid.uuid4()) + '\n')
                stream.flush()
                os.fsync(stream.fileno())
            try:
                os.link(temporary, path)
            except FileExistsError:
                pass
        finally:
            os.unlink(temporary)
    value = path.read_text(encoding='ascii').strip()
    if str(uuid.UUID(value)) != value:
        raise ValueError('Invalid existing server-id; restore it from backup')
    return value


def migrate_file_origins(data_dir, old_origin, new_origin):
    """Run before accepting requests. Preserve objects/IDs and backup every edit.

    Only Parse File URLs for files actually present in this store are changed.
    A manifest is written last; interrupted runs resume without replacing backups.
    """
    data = Path(data_dir)
    previous, current = origin(old_origin), origin(new_origin)
    if previous == current:
        raise ValueError('Migration origins must differ')
    identity = server_id(data)
    key = hashlib.sha256((previous + '\n' + current).encode()).hexdigest()[:24]
    backup = data / 'migrations' / ('file-origin-' + key)
    marker = backup / 'complete.json'
    if marker.exists():
        result = json.loads(marker.read_text())
        if result['serverId'] != identity:
            raise ValueError('Migration belongs to another server identity')
        return result
    plan = []

    def rewrite(value):
        changed = False
        if isinstance(value, dict):
            url = value.get('url')
            name = value.get('name')
            if value.get('__type') == 'File' and isinstance(url, str) and isinstance(name, str):
                parsed = urlsplit(url)
                # Match exactly /files/<encoded name>, not unrelated links or audio paths.
                from urllib.parse import quote
                expected = '/files/' + quote(name, safe='')
                if (not parsed.query and not parsed.fragment and parsed.path == expected
                        and origin(parsed.scheme + '://' + parsed.netloc) == previous
                        and Path(name).name == name and (data / 'files' / name).is_file()):
                    value['url'] = current + parsed.path
                    changed = True
            for child in value.values():
                changed = rewrite(child) or changed
        elif isinstance(value, list):
            for child in value:
                changed = rewrite(child) or changed
        return changed

    classes = data / 'classes'
    if classes.exists():
        for path in sorted(classes.glob('*/*.json')):
            if path.is_symlink():
                raise ValueError('Refusing a symlinked object: ' + str(path))
            raw = path.read_bytes()
            obj = json.loads(raw)
            if rewrite(obj):
                plan.append((path, raw, (json.dumps(obj, ensure_ascii=False, indent=2) + '\n').encode()))
    backup.mkdir(parents=True, exist_ok=True)
    for path, raw, replacement in plan:
        saved = backup / path.relative_to(data)
        saved.parent.mkdir(parents=True, exist_ok=True)
        # An interrupted run may already have its original backup. Keep it.
        if not saved.exists():
            with saved.open('xb') as stream:
                stream.write(raw)
                stream.flush()
                os.fsync(stream.fileno())
        atomic_write(path, replacement)
    result = {'serverId': identity, 'from': previous, 'to': current,
              'changedObjects': len(list((backup / 'classes').glob('*/*.json')))}
    atomic_write(marker, (json.dumps(result, indent=2) + '\n').encode())
    return result
