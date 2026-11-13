"""Install a release-verified Flask backend beside the private Windows Python."""
import os
from pathlib import PurePosixPath
import shutil
import stat
import subprocess
import tempfile
import zipfile


def install(server, source):
    from local_server import atomic
    if not server.python.is_file():
        raise RuntimeError('Install the private Python runtime through ReRiff setup first.')
    package = server.image.parent
    if package.is_symlink():
        raise ValueError('The optional server application must not be a directory link.')
    server.root.mkdir(parents=True, exist_ok=True)
    stage = tempfile.mkdtemp(prefix='.windows-server-', dir=server.root)
    from pathlib import Path
    stage = Path(stage)
    previous = stage.with_name(stage.name+'.previous')
    old_config = server.config_path.read_bytes() if server.config_path.exists() else None
    moved = False
    try:
        total, names = 0, set()
        with zipfile.ZipFile(source) as bundle:
            if len(bundle.infolist()) > 10000:
                raise ValueError('Local server archive has too many entries.')
            for info in bundle.infolist():
                # Windows ZipInfo normalizes backslashes in filename; inspect
                # the original archive spelling before platform normalization.
                raw_name = info.orig_filename
                relative = PurePosixPath(raw_name)
                key = relative.as_posix().casefold()
                # Same relative-path/link restrictions as the existing updater,
                # with Windows aliases rejected before writing any file.
                if (relative.is_absolute() or '..' in relative.parts or '\\' in raw_name
                        or ':' in raw_name or key in names or not relative.parts
                        or stat.S_IFMT(info.external_attr >> 16) == stat.S_IFLNK
                        or any(part.endswith((' ', '.')) or part.split('.')[0].upper() in
                               {'CON','PRN','AUX','NUL',*[f'COM{i}' for i in range(1,10)],*[f'LPT{i}' for i in range(1,10)]}
                               for part in relative.parts)):
                    raise ValueError('Unsafe local server archive path.')
                names.add(key);total += info.file_size
                if total > 256*1024*1024:
                    raise ValueError('Local server archive is too large.')
                output = stage.joinpath(*relative.parts)
                if info.is_dir():
                    output.mkdir(parents=True, exist_ok=True)
                else:
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_bytes(bundle.read(info))
        for required in ('entry.py', 'local_server.py', 'serve.py', 'host_process.py', 'server/riffapi-local.py',
                         'server/reriff-catalog.dll', 'server/reriff-news.dll', 'python-libs/flask/__init__.py', 'python-libs/waitress/__init__.py'):
            if not (stage/required).is_file():
                raise ValueError('Local server download is incomplete: '+required)
        check = subprocess.run([str(server.python), '-I', str(stage/'entry.py'), '--self-test'],
                               capture_output=True, text=True, timeout=60, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        if check.returncode:
            raise RuntimeError('Downloaded server failed its self-test: '+check.stderr[-1000:])
        if package.exists():
            os.replace(package, previous)
        os.replace(stage, package);moved = True
        try:
            import json
            atomic(server.config_path, json.dumps(server.config, indent=2)+'\n')
        except BaseException:
            shutil.rmtree(package)
            if previous.exists():
                os.replace(previous, package)
            if old_config is not None:
                server.config_path.write_bytes(old_config)
            raise
        return 'Local server installed. Use '+server.address+' in Secondary server.'
    except BaseException:
        if not moved and previous.exists() and not package.exists():
            os.replace(previous, package)
        raise
    finally:
        if stage.exists():
            shutil.rmtree(stage)
        if moved and previous.exists():
            shutil.rmtree(previous)
