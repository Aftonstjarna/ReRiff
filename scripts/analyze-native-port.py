#!/usr/bin/env python3
"""Read-only executable/asset survey. Requires dnfile, dncil, and pefile.

Outputs are local research material, not release inputs.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import dnfile
from dncil.cil.body.reader import read_method_body_from_bytes
from dncil.clr.token import Token


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('analysis/native-port'))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    executable = args.game_dir / 'Game.exe'
    pe = dnfile.dnPE(str(executable))

    def resolve(token):
        if token.table == 0x70:
            return repr(pe.net.user_strings.get(token.rid).value)
        table = pe.net.mdtables.tables.get(token.table)
        if table is None or not 1 <= token.rid <= len(table.rows):
            return str(token)
        row = table.rows[token.rid - 1]
        return str(getattr(row, 'Name', getattr(row, 'TypeName', token)))

    methods = []
    groups = collections.defaultdict(list)
    for rid, method in enumerate(pe.net.mdtables.MethodDef.rows, 1):
        name = str(method.Name)
        kind = 'IL' if method.ImplFlags.miIL else 'native' if method.ImplFlags.miNative else 'runtime'
        item = dict(token=f'0x{0x06000000 + rid:08x}', name=name,
                    rva=method.Rva, va=pe.OPTIONAL_HEADER.ImageBase + method.Rva,
                    implementation=kind, signature=method.Signature.value.hex())
        methods.append(item)
        # Preserve offsets, instructions, token identities, locals, and EH data.
        # This is a disassembly, not reconstructed C++ source.
        if method.Rva and kind == 'IL' and name.startswith(('ga', 'gf')):
            try:
                body = read_method_body_from_bytes(pe.get_data(method.Rva))
                lines = [f'// {name} {item["token"]} RVA 0x{method.Rva:x}',
                         f'// signature {item["signature"]}; locals {body.local_var_sig_tok}']
                for instruction in body.instructions:
                    operand = instruction.operand
                    if isinstance(operand, Token):
                        operand = f'{operand} /* {resolve(operand)} */'
                    lines.append(f'{instruction.offset:04x}: {instruction.opcode.name:16} {operand if operand is not None else ""}')
                for handler in body.exception_handlers:
                    lines.append('// EH ' + repr(vars(handler)))
                groups[name.split('.', 1)[0]].append('\n'.join(lines))
            except Exception as error:
                item['disassembly_error'] = str(error)
    (args.output / 'methods.json').write_text(json.dumps(methods, indent=2) + '\n')
    il_dir = args.output / 'il'
    il_dir.mkdir(exist_ok=True)
    for group, bodies in groups.items():
        safe = ''.join(c if c.isalnum() or c in '_-' else '_' for c in group)
        (il_dir / (safe + '.il')).write_text('\n\n'.join(bodies) + '\n')
    with zipfile.ZipFile(args.game_dir / 'vfs.zip') as archive:
        assets = []
        for entry in archive.infolist():
            with archive.open(entry) as stream:
                magic = stream.read(16)
            assets.append(dict(path=entry.filename, size=entry.file_size,
                               compressed_size=entry.compress_size, crc32=f'{entry.CRC:08x}',
                               header=magic.hex()))
    (args.output / 'assets.json').write_text(json.dumps(assets, indent=2) + '\n')
    clr_rva = pe.OPTIONAL_HEADER.DATA_DIRECTORY[14].VirtualAddress
    report = dict(executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                  machine=f'0x{pe.FILE_HEADER.Machine:04x}',
                  image_base=pe.OPTIONAL_HEADER.ImageBase,
                  clr_flags=struct.unpack_from('<I', pe.get_data(clr_rva, 72), 16)[0],
                  method_counts=dict(collections.Counter(m['implementation'] for m in methods)),
                  il_disassembly_failures=sum('disassembly_error' in m for m in methods),
                  disassembled_methods=sum(map(len, groups.values())),
                  imports=[dict(dll=d.dll.decode(), symbols=[i.name.decode() if i.name else f'ordinal:{i.ordinal}' for i in d.imports]) for d in pe.DIRECTORY_ENTRY_IMPORT],
                  assets=len(assets), asset_extensions=dict(collections.Counter(Path(a['path']).suffix for a in assets)))
    (args.output / 'survey.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'imports'}, indent=2))


if __name__ == '__main__':
    main()
