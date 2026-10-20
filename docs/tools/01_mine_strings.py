import re

def extract_strings(path, min_len=4):
    with open(path, 'rb') as f:
        data = f.read()
    # ASCII strings
    ascii_re = re.compile(rb'[\x20-\x7e]{%d,}' % min_len)
    strs = set(m.group().decode() for m in ascii_re.finditer(data))
    # UTF-16LE strings
    u16_re = re.compile(rb'(?:[\x20-\x7e]\x00){%d,}' % min_len)
    for m in u16_re.finditer(data):
        strs.add(m.group().decode('utf-16-le'))
    return strs

keywords = ['ghost', 'Ghost', 'GHOST', 'score', 'Score', 'leaderboard', 'Leaderboard',
            'track', 'Track', 'parse', 'Parse', 'classes/', '/files/', 'objectId',
            'query', 'Query', 'where', 'limit', 'order', 'descending', 'ascending',
            'upload', 'Upload', 'download', 'Download', 'save', 'Save', 'fetch',
            'user', 'User', 'session', 'Session', 'installation', 'Installation',
            'X-Parse', 'api.parse', 'back4app', 'parseapi']

for path in [r'C:\Games\Riff Racer\Game.exe',
             r'C:\Games\Riff Racer\ParseOctane.dll',
             r'C:\Games\Riff Racer\Parse.dll']:
    print('=' * 30)
    print(path)
    print('=' * 30)
    strs = extract_strings(path)
    hits = []
    for s in strs:
        low = s.lower()
        if any(k.lower() in low for k in keywords):
            hits.append(s)
    hits = sorted(set(hits))
    for h in hits:
        print(repr(h))
    print(f'({len(hits)} hits)')