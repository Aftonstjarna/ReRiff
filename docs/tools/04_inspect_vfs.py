import zipfile
from collections import Counter

z = zipfile.ZipFile(r'C:\Games\Riff Racer\vfs.zip')
names = z.namelist()
interesting = [n for n in names if n.endswith(('.dll', '.exe', '.config', '.xml', '.json', '.txt')) or 'code' in n.lower() or 'script' in n.lower() or 'managed' in n.lower()]
print('INTERESTING:', len(interesting))
for n in interesting:
    print(n)
print()
print('EXT COUNTS:')
c = Counter(n.rsplit('.', 1)[-1] if '.' in n else '(none)' for n in names)
for k, v in c.most_common(40):
    print(k, v)