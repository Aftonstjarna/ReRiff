"""Explicit identities for published packages; keep artifacts for old updaters."""
import hashlib
from pathlib import Path


def release_metadata(output, version, commit):
    output = Path(output)
    # Classification is explicit at publication, never inferred by consumers
    # from suffixes shared by client and optional server downloads.
    specs = [
        ('client', 'windows', 'x86', f'ReRiff-{version}-windows.zip'),
        ('client', 'linux', 'x86_64', f'ReRiff-{version}-linux.run'),
        ('server', 'windows', 'x86_64', 'ReRiff-Windows-Server.zip'),
        ('server', 'linux', 'x86_64', 'ReRiff-Server-x86_64.AppImage'),
        ('fonts-cjk', 'any', 'any', 'ReRiff-CJK.zip'),
        ('fonts-latin', 'any', 'any', 'ReRiff-Latin.zip'),
        ('source', 'any', 'any', f'ReRiff-{version}-source.tar.gz'),
    ]
    packages = []
    for component, platform, architecture, filename in specs:
        path = output/filename
        if not path.is_file() or path.is_symlink() or path.stat().st_size <= 0:
            raise ValueError('Missing or invalid release package: '+filename)
        digest = hashlib.sha256()
        with path.open('rb') as stream:
            for chunk in iter(lambda: stream.read(1024*1024), b''):
                digest.update(chunk)
        packages.append(dict(component=component, platform=platform,
                             architecture=architecture, filename=filename,
                             size=path.stat().st_size, sha256=digest.hexdigest()))
    return dict(schema=2, version=version, commit=commit,
                artifacts=sorted(p['filename'] for p in packages), packages=packages)
