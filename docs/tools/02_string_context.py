import re

path = r'C:\Games\Riff Racer\Game.exe'
with open(path, 'rb') as f:
    data = f.read()

# Extract ASCII strings with offsets
ascii_re = re.compile(rb'[\x20-\x7e]{4,}')
strs = [(m.start(), m.group().decode()) for m in ascii_re.finditer(data)]

# Find indices of interesting anchor strings
anchors = ['Ghost', 'Uploader', 'PlaylistTracks', 'Score', 'Track', 'objectId',
           'pendingGhostUploads', 'parseAppKey', 'Writing song metadata',
           'Writing playlist entry', 'LeaderboardEntry', 'ScoreOverthrownCheck',
           'PARSEOP', 'QUERY', 'Where', 'where']

for anchor in anchors:
    idxs = [i for i, (off, s) in enumerate(strs) if s == anchor]
    if not idxs:
        # try substring
        idxs = [i for i, (off, s) in enumerate(strs) if anchor in s][:3]
    for i in idxs[:2]:
        print(f'--- context around {anchor!r} (offset {hex(strs[i][0])}) ---')
        lo = max(0, i - 25)
        hi = min(len(strs), i + 25)
        for j in range(lo, hi):
            marker = '>>' if j == i else '  '
            print(f'{marker} {hex(strs[j][0])}: {strs[j][1][:100]!r}')
        print()