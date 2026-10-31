"""Small hash-verified binary deltas. Releases do not contain original game DLLs."""
import base64
import hashlib
import json
import zlib


def digest(data):
    return hashlib.sha256(data).hexdigest()


def make_patch(original, modified):
    index = {}
    for offset in range(max(0, len(original)-15)):
        entries = index.setdefault(original[offset:offset+16], [])
        if len(entries) < 8:
            entries.append(offset)
    ops, literal = [], bytearray()
    position = 0
    while position < len(modified):
        best, start = 0, 0
        for candidate in index.get(modified[position:position+16], ()):
            length = 16
            while candidate+length < len(original) and position+length < len(modified) and original[candidate+length] == modified[position+length]:
                length += 1
            if length > best:
                best, start = length, candidate
        if best >= 16:
            if literal:
                ops.append({'add': base64.b64encode(literal).decode()});literal.clear()
            ops.append({'copy': start, 'size': best})
            position += best
        else:
            literal.append(modified[position]);position += 1
    if literal:
        ops.append({'add': base64.b64encode(literal).decode()})
    return zlib.compress(json.dumps({'format': 1, 'original': digest(original), 'patched': digest(modified), 'ops': ops}, separators=(',', ':')).encode(), 9)


def apply_patch(original, encoded):
    patch = json.loads(zlib.decompress(encoded))
    if patch.get('format') != 1 or digest(original) != patch['original']:
        raise ValueError('Unsupported original DLL; no patch applied.')
    output = bytearray()
    for op in patch['ops']:
        if 'copy' in op:
            start, length = op['copy'], op['size']
            if not isinstance(start, int) or not isinstance(length, int) or start < 0 or length < 0 or start+length > len(original):
                raise ValueError('Invalid binary patch range.')
            output.extend(original[start:start+length])
        else:
            output.extend(base64.b64decode(op['add'], validate=True))
    if digest(output) != patch['patched']:
        raise ValueError('Patched DLL verification failed.')
    return bytes(output)
