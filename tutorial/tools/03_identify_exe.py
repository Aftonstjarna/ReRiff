import struct

# Check Game.exe PE header
with open(r'C:\Games\Riff Racer\Game.exe', 'rb') as f:
    data = f.read()
print('Game.exe size:', len(data))
print('MZ:', data[:2])
pe_off = struct.unpack_from('<I', data, 0x3C)[0]
print('PE offset:', hex(pe_off), 'sig:', data[pe_off:pe_off+4])
machine = struct.unpack_from('<H', data, pe_off + 4)[0]
print('Machine:', hex(machine))  # 0x14c=x86, 0x8664=x64
opt_off = pe_off + 24
magic = struct.unpack_from('<H', data, opt_off)[0]
print('Opt magic:', hex(magic))  # 0x10b=PE32, 0x20b=PE32+
# .NET directory: data directory index 14
if magic == 0x10b:
    dd_off = opt_off + 96
else:
    dd_off = opt_off + 112
net_rva, net_size = struct.unpack_from('<II', data, dd_off + 14 * 8)
print('.NET dir RVA:', hex(net_rva), 'size:', net_size, '-> IS .NET' if net_rva else '-> native')
# subsystem
subsys = struct.unpack_from('<H', data, opt_off + 68)[0]
print('Subsystem:', subsys)  # 2=GUI, 3=console

# strings in Game.exe related to parse/ghost
import re
for pat in [b'parseapi', b'back4app', b'[Gg]host', b'Parse']:
    matches = re.findall(pat, data)
    print(pat, len(matches))

# vfs.dat header
with open(r'C:\Games\Riff Racer\vfs.dat', 'rb') as f:
    vdat = f.read(4096)
print()
print('vfs.dat size:', len(open(r"e:\\tmp\\BrokenCopies\\Riff Racer\\vfs.dat","rb").read()))
print('vfs.dat first 64 bytes:', vdat[:64].hex())
print('vfs.dat ascii:', ''.join(chr(b) if 32 <= b < 127 else '.' for b in vdat[:128]))