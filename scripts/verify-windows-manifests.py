"""Verify embedded execution-level resources in the built Windows programs."""
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
for name in ('ReRiff-Launcher.exe', 'ReRiff-Updater.exe', 'ReRiff-Setup.exe'):
    data = (root/'build'/name).read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe+4] == b'PE\0\0'
    sections, optional_size = struct.unpack_from('<H', data, pe+6)[0], struct.unpack_from('<H', data, pe+20)[0]
    optional = pe+24
    directories = optional+(96 if struct.unpack_from('<H', data, optional)[0] == 0x10b else 112)
    resource_rva = struct.unpack_from('<I', data, directories+16)[0]
    def offset(rva):
        for index in range(sections):
            entry = optional+optional_size+40*index
            size, start, raw_size, raw = struct.unpack_from('<IIII', data, entry+8)
            if start <= rva < start+max(size, raw_size):
                return raw+rva-start
        raise AssertionError(f'{name}: RVA outside sections')
    resource = offset(resource_rva)
    def entries(relative):
        start = resource+relative
        named, ids = struct.unpack_from('<HH', data, start+12)
        return [struct.unpack_from('<II', data, start+16+8*i) for i in range(named+ids)]
    manifest_type = next(value for key, value in entries(0) if key == 24)
    assert manifest_type & 0x80000000
    manifest_id = next(value for key, value in entries(manifest_type & 0x7fffffff) if key == 1)
    assert manifest_id & 0x80000000
    _, leaf = entries(manifest_id & 0x7fffffff)[0]
    assert not leaf & 0x80000000
    rva, size = struct.unpack_from('<II', data, resource+leaf)
    manifest = ET.fromstring(data[offset(rva):offset(rva)+size].rstrip(b'\0'))
    execution = next(node for node in manifest.iter() if node.tag.endswith('requestedExecutionLevel'))
    assert execution.attrib == {'level': 'asInvoker', 'uiAccess': 'false'}, name
    print(f'PASS: {name} embeds asInvoker execution level')
