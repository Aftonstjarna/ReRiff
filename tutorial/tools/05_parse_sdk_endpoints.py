import re

for path in [r'C:\Games\Riff Racer\Parse.dll', r'C:\Games\Riff Racer\Parse.NetFx45.dll']:
    print('=' * 40)
    print(path)
    with open(path, 'rb') as f:
        data = f.read()
    print('size:', len(data))
    # UTF-16 strings (typical for .NET metadata string heap? No, that's UTF-8; but literals are UTF-16)
    u16_re = re.compile(rb'(?:[\x20-\x7e]\x00){3,}')
    seen = set()
    out = []
    for m in u16_re.finditer(data):
        s = m.group().decode('utf-16-le')
        if s in seen:
            continue
        seen.add(s)
        low = s.lower()
        if any(k in low for k in ['classes', '/files', '/batch', 'x-parse', 'parse/', 'api.parse',
                                   'application/json', 'octet-stream', 'content-type', 'delete',
                                   'login', 'users', 'installations', 'events', 'push', 'request',
                                   'where', 'limit', 'order', 'count', 'include', 'keys',
                                   'sessiontoken', 'revision', 'sdk']):
            out.append(s)
    for s in sorted(out):
        print(repr(s))