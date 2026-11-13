"""Write font-only native VFS volumes; never edit the original game archive.

Format evidence and validation: reports/CJK-FONT-INTEGRATION.md.
"""
from pathlib import Path, PurePosixPath
import struct
import zipfile


def build_volume(source: Path, output: Path, stem: str):
    if not stem.startswith('vfs') or not stem[3:] or len(stem[3:]) > 7:
        raise ValueError('Additional native volume requires vfs<nonzero hex id>')
    volume = int(stem[3:], 16)
    if not 0 < volume <= 31:
        raise ValueError('Use a free volume id from 1 through 31')
    files = {}
    for path in source.rglob('*'):
        if path.is_symlink():
            raise ValueError('Asset symlinks are not accepted')
        if path.is_file():
            name = path.relative_to(source).as_posix()
            if not name.isascii() or name != name.lower() or '\\' in name:
                raise ValueError('Native resource paths must be lower-case ASCII')
            if name in files:
                raise ValueError('Duplicate native resource')
            files[name] = path
    if not files or len(files) >= 0x3fff:
        raise ValueError('Invalid native volume file count')
    output.mkdir(parents=True, exist_ok=True)
    archive = output/(stem+'.zip')
    # Owned build outputs only; callers publish/install the pair atomically.
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED,
                         compresslevel=9, allowZip64=False) as bundle:
        for name, path in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            bundle.writestr(info, path.read_bytes())
    with zipfile.ZipFile(archive) as bundle:
        entries = bundle.infolist()
    pool = bytearray()
    nodes = []
    children = {}
    paths = {}
    for file_index, info in enumerate(entries):
        parts = PurePosixPath(info.filename).parts
        parent = ''
        for depth, part in enumerate(parts):
            path = '/'.join(parts[:depth+1])
            if path not in paths:
                if len(pool)+len(part)+1 > 65535 or len(nodes) >= 65535:
                    raise ValueError('Native VFS tree exceeds 16-bit bounds')
                index = len(nodes)
                paths[path] = index
                nodes.append([len(pool), 0xffff, 0xffff,
                              file_index if depth == len(parts)-1 else 0x3fff])
                pool.extend(part.encode('ascii')+b'\0')
                children.setdefault(parent, []).append(index)
            elif nodes[paths[path]][3] != 0x3fff:
                raise ValueError('Asset file conflicts with a directory')
            parent = path
    for parent, indices in children.items():
        if parent:
            nodes[paths[parent]][2] = indices[0]
        for current, following in zip(indices, indices[1:]):
            nodes[current][1] = following
    crc_total = sum(info.CRC for info in entries) & 0xffffffff
    header = struct.pack('<5I4H', 0x0300ffee, len(nodes), len(entries), len(pool),
                         crc_total, 0, 0xffff, children[''][0], 0xffff)
    records = bytearray()
    central_offset = 0
    for info in entries:
        name_size = len(info.filename.encode('ascii'))
        records.extend(struct.pack('<7I', 2, info.compress_type, info.compress_size,
                                   info.file_size, name_size, info.header_offset, central_offset))
        central_offset += 46+name_size+len(info.extra)+len(info.comment)
    index = output/(stem+'.dat')
    index.write_bytes(header+b''.join(struct.pack('<4H', *node) for node in nodes)
                      +records+pool)
    return index, archive
