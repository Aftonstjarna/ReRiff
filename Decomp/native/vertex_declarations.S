# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a3a40..0x4a4c43
.intel_syntax noprefix
.code32
.text
.global original_vertex_declarations
original_vertex_declarations:
.L4a3a40:
 push   ebp
.L4a3a41:
 {load} mov    ebp,esp
.L4a3a43:
 push   0xffffffff
.L4a3a45:
 push   0x902acb
.L4a3a4a:
 mov    eax,fs:0x0
.L4a3a50:
 push   eax
.L4a3a51:
 sub    esp,0x674
.L4a3a57:
 mov    eax,[0xbaabc0]
.L4a3a5c:
 {load} xor    eax,ebp
.L4a3a5e:
 mov    DWORD PTR [ebp-0x10],eax
.L4a3a61:
 push   ebx
.L4a3a62:
 push   esi
.L4a3a63:
 push   edi
.L4a3a64:
 push   eax
.L4a3a65:
 lea    eax,[ebp-0xc]
.L4a3a68:
 mov    fs:0x0,eax
.L4a3a6e:
 {load} mov    ebx,ecx
.L4a3a70:
 {load} xor    eax,eax
.L4a3a72:
 mov    WORD PTR [ebp-0x67c],0x2
.L4a3a7b:
 mov    DWORD PTR [ebp-0x680],eax
.L4a3a81:
 mov    DWORD PTR [ebp-0x67a],0xff0000
.L4a3a8b:
 mov    DWORD PTR [ebp-0x676],0x110000
.L4a3a95:
 mov    WORD PTR [ebp-0x672],ax
.L4a3a9c:
 mov    DWORD PTR [ebx+0x31c],0xc
.L4a3aa6:
 mov    DWORD PTR [ebp-0x40],eax
.L4a3aa9:
 mov    WORD PTR [ebp-0x3c],0x2
.L4a3aaf:
 mov    DWORD PTR [ebp-0x3a],eax
.L4a3ab2:
 mov    DWORD PTR [ebp-0x36],0x2000c
.L4a3ab9:
 mov    DWORD PTR [ebp-0x32],0xff0003
.L4a3ac0:
 mov    DWORD PTR [ebp-0x2e],0x110000
.L4a3ac7:
 mov    WORD PTR [ebp-0x2a],ax
.L4a3acb:
 mov    DWORD PTR [ebx+0x320],0x18
.L4a3ad5:
 mov    DWORD PTR [ebp-0xb8],eax
.L4a3adb:
 mov    WORD PTR [ebp-0xb4],0x2
.L4a3ae4:
 mov    DWORD PTR [ebp-0xb2],eax
.L4a3aea:
 mov    DWORD PTR [ebp-0xae],0x2000c
.L4a3af4:
 mov    DWORD PTR [ebp-0xaa],0x3
.L4a3afe:
 mov    DWORD PTR [ebp-0xa6],0x10018
.L4a3b08:
 mov    DWORD PTR [ebp-0xa2],0xff0005
.L4a3b12:
 mov    DWORD PTR [ebp-0x9e],0x110000
.L4a3b1c:
 mov    WORD PTR [ebp-0x9a],ax
.L4a3b23:
 mov    DWORD PTR [ebx+0x324],0x20
.L4a3b2d:
 mov    DWORD PTR [ebp-0x178],eax
.L4a3b33:
 mov    WORD PTR [ebp-0x174],0x2
.L4a3b3c:
 mov    DWORD PTR [ebp-0x172],eax
.L4a3b42:
 mov    DWORD PTR [ebp-0x16e],0x2000c
.L4a3b4c:
 mov    DWORD PTR [ebp-0x16a],0x3
.L4a3b56:
 mov    DWORD PTR [ebp-0x166],0x10018
.L4a3b60:
 mov    DWORD PTR [ebp-0x162],0x5
.L4a3b6a:
 mov    DWORD PTR [ebp-0x15e],0x10020
.L4a3b74:
 mov    DWORD PTR [ebp-0x15a],0xff0105
.L4a3b7e:
 mov    DWORD PTR [ebp-0x156],0x110000
.L4a3b88:
 mov    WORD PTR [ebp-0x152],ax
.L4a3b8f:
 mov    DWORD PTR [ebx+0x328],0x28
.L4a3b99:
 mov    DWORD PTR [ebp-0x58],eax
.L4a3b9c:
 mov    WORD PTR [ebp-0x54],0x2
.L4a3ba2:
 mov    DWORD PTR [ebp-0x52],eax
.L4a3ba5:
 mov    DWORD PTR [ebp-0x4e],0x4000c
.L4a3bac:
 mov    DWORD PTR [ebp-0x4a],0xff000a
.L4a3bb3:
 mov    DWORD PTR [ebp-0x46],0x110000
.L4a3bba:
 mov    WORD PTR [ebp-0x42],ax
.L4a3bbe:
 mov    DWORD PTR [ebx+0x32c],0x10
.L4a3bc8:
 mov    DWORD PTR [ebp-0x98],eax
.L4a3bce:
 mov    WORD PTR [ebp-0x94],0x2
.L4a3bd7:
 mov    DWORD PTR [ebp-0x92],eax
.L4a3bdd:
 mov    DWORD PTR [ebp-0x8e],0x1000c
.L4a3be7:
 mov    DWORD PTR [ebp-0x8a],0x5
.L4a3bf1:
 mov    DWORD PTR [ebp-0x86],0x40014
.L4a3bfb:
 mov    DWORD PTR [ebp-0x82],0xff000a
.L4a3c05:
 mov    DWORD PTR [ebp-0x7e],0x110000
.L4a3c0c:
 mov    WORD PTR [ebp-0x7a],ax
.L4a3c10:
 mov    DWORD PTR [ebx+0x330],0x18
.L4a3c1a:
 mov    DWORD PTR [ebp-0x218],eax
.L4a3c20:
 mov    WORD PTR [ebp-0x214],0x2
.L4a3c29:
 mov    DWORD PTR [ebp-0x212],eax
.L4a3c2f:
 mov    DWORD PTR [ebp-0x20e],0x1000c
.L4a3c39:
 mov    DWORD PTR [ebp-0x20a],0x5
.L4a3c43:
 mov    DWORD PTR [ebp-0x206],0x10014
.L4a3c4d:
 mov    DWORD PTR [ebp-0x202],0x105
.L4a3c57:
 mov    DWORD PTR [ebp-0x1fe],0x4001c
.L4a3c61:
 mov    DWORD PTR [ebp-0x1fa],0xff000a
.L4a3c6b:
 mov    DWORD PTR [ebp-0x1f6],0x110000
.L4a3c75:
 mov    WORD PTR [ebp-0x1f2],ax
.L4a3c7c:
 mov    DWORD PTR [ebx+0x334],0x20
.L4a3c86:
 mov    DWORD PTR [ebp-0x128],eax
.L4a3c8c:
 mov    WORD PTR [ebp-0x124],0x2
.L4a3c95:
 mov    DWORD PTR [ebp-0x122],eax
.L4a3c9b:
 mov    DWORD PTR [ebp-0x11e],0x2000c
.L4a3ca5:
 mov    DWORD PTR [ebp-0x11a],0x3
.L4a3caf:
 mov    DWORD PTR [ebp-0x116],0x10018
.L4a3cb9:
 mov    DWORD PTR [ebp-0x112],0x5
.L4a3cc3:
 mov    DWORD PTR [ebp-0x10e],0x40020
.L4a3ccd:
 mov    DWORD PTR [ebp-0x10a],0xff000a
.L4a3cd7:
 mov    DWORD PTR [ebp-0x106],0x110000
.L4a3ce1:
 mov    WORD PTR [ebp-0x102],ax
.L4a3ce8:
 mov    DWORD PTR [ebx+0x338],0x24
.L4a3cf2:
 mov    DWORD PTR [ebp-0x330],eax
.L4a3cf8:
 mov    WORD PTR [ebp-0x32c],0x2
.L4a3d01:
 mov    DWORD PTR [ebp-0x32a],eax
.L4a3d07:
 mov    DWORD PTR [ebp-0x326],0x2000c
.L4a3d11:
 mov    DWORD PTR [ebp-0x322],0x3
.L4a3d1b:
 mov    DWORD PTR [ebp-0x31e],0x10018
.L4a3d25:
 mov    DWORD PTR [ebp-0x31a],0x5
.L4a3d2f:
 mov    DWORD PTR [ebp-0x316],0x10020
.L4a3d39:
 mov    DWORD PTR [ebp-0x312],0x105
.L4a3d43:
 mov    DWORD PTR [ebp-0x30e],0x40028
.L4a3d4d:
 mov    DWORD PTR [ebp-0x30a],0xff000a
.L4a3d57:
 mov    DWORD PTR [ebp-0x306],0x110000
.L4a3d61:
 mov    WORD PTR [ebp-0x302],ax
.L4a3d68:
 mov    DWORD PTR [ebx+0x33c],0x2c
.L4a3d72:
 mov    DWORD PTR [ebp-0x150],eax
.L4a3d78:
 mov    WORD PTR [ebp-0x14c],0x2
.L4a3d81:
 mov    DWORD PTR [ebp-0x14a],eax
.L4a3d87:
 mov    DWORD PTR [ebp-0x146],0x1000c
.L4a3d91:
 mov    DWORD PTR [ebp-0x142],0x5
.L4a3d9b:
 mov    DWORD PTR [ebp-0x13e],0x40014
.L4a3da5:
 mov    DWORD PTR [ebp-0x13a],0xa
.L4a3daf:
 mov    DWORD PTR [ebp-0x136],0x30018
.L4a3db9:
 mov    DWORD PTR [ebp-0x132],0xff0006
.L4a3dc3:
 mov    DWORD PTR [ebp-0x12e],0x110000
.L4a3dcd:
 mov    WORD PTR [ebp-0x12a],ax
.L4a3dd4:
 mov    DWORD PTR [ebx+0x340],0x28
.L4a3dde:
 mov    DWORD PTR [ebp-0x1a0],eax
.L4a3de4:
 mov    WORD PTR [ebp-0x19c],0x2
.L4a3ded:
 mov    DWORD PTR [ebp-0x19a],eax
.L4a3df3:
 mov    DWORD PTR [ebp-0x196],0x1000c
.L4a3dfd:
 mov    DWORD PTR [ebp-0x192],0x5
.L4a3e07:
 mov    DWORD PTR [ebp-0x18e],0x40014
.L4a3e11:
 mov    DWORD PTR [ebp-0x18a],0xa
.L4a3e1b:
 mov    DWORD PTR [ebp-0x186],0x18
.L4a3e25:
 mov    DWORD PTR [ebp-0x182],0xff0001
.L4a3e2f:
 mov    DWORD PTR [ebp-0x17e],0x110000
.L4a3e39:
 mov    WORD PTR [ebp-0x17a],ax
.L4a3e40:
 mov    DWORD PTR [ebx+0x344],0x1c
.L4a3e4a:
 mov    DWORD PTR [ebp-0x28],eax
.L4a3e4d:
 mov    WORD PTR [ebp-0x24],0x2
.L4a3e53:
 mov    DWORD PTR [ebp-0x22],eax
.L4a3e56:
 mov    DWORD PTR [ebp-0x1e],0x10000c
.L4a3e5d:
 mov    DWORD PTR [ebp-0x1a],0xff0003
.L4a3e64:
 mov    DWORD PTR [ebp-0x16],0x110000
.L4a3e6b:
 mov    WORD PTR [ebp-0x12],ax
.L4a3e6f:
 mov    DWORD PTR [ebx+0x348],0x14
.L4a3e79:
 mov    DWORD PTR [ebp-0xd8],eax
.L4a3e7f:
 mov    WORD PTR [ebp-0xd4],0x2
.L4a3e88:
 mov    DWORD PTR [ebp-0xd2],eax
.L4a3e8e:
 mov    DWORD PTR [ebp-0xce],0x10000c
.L4a3e98:
 mov    DWORD PTR [ebp-0xca],0x3
.L4a3ea2:
 mov    DWORD PTR [ebp-0xc6],0xf0014
.L4a3eac:
 mov    DWORD PTR [ebp-0xc2],0xff0005
.L4a3eb6:
 mov    DWORD PTR [ebp-0xbe],0x110000
.L4a3ec0:
 mov    WORD PTR [ebp-0xba],ax
.L4a3ec7:
 mov    DWORD PTR [ebx+0x34c],0x18
.L4a3ed1:
 mov    DWORD PTR [ebp-0x1f0],eax
.L4a3ed7:
 mov    WORD PTR [ebp-0x1ec],0x2
.L4a3ee0:
 mov    DWORD PTR [ebp-0x1ea],eax
.L4a3ee6:
 mov    DWORD PTR [ebp-0x1e6],0x10000c
.L4a3ef0:
 mov    DWORD PTR [ebp-0x1e2],0x3
.L4a3efa:
 mov    DWORD PTR [ebp-0x1de],0xf0014
.L4a3f04:
 mov    DWORD PTR [ebp-0x1da],0x5
.L4a3f0e:
 mov    DWORD PTR [ebp-0x1d6],0xf0018
.L4a3f18:
 mov    DWORD PTR [ebp-0x1d2],0xff0105
.L4a3f22:
 mov    DWORD PTR [ebp-0x1ce],0x110000
.L4a3f2c:
 mov    WORD PTR [ebp-0x1ca],ax
.L4a3f33:
 mov    DWORD PTR [ebx+0x350],0x1c
.L4a3f3d:
 mov    DWORD PTR [ebp-0x78],eax
.L4a3f40:
 mov    WORD PTR [ebp-0x74],0x2
.L4a3f46:
 mov    DWORD PTR [ebp-0x72],eax
.L4a3f49:
 mov    DWORD PTR [ebp-0x6e],0xf000c
.L4a3f50:
 mov    DWORD PTR [ebp-0x6a],0x5
.L4a3f57:
 mov    DWORD PTR [ebp-0x66],0x40010
.L4a3f5e:
 mov    DWORD PTR [ebp-0x62],0xff000a
.L4a3f65:
 mov    DWORD PTR [ebp-0x5e],0x110000
.L4a3f6c:
 mov    WORD PTR [ebp-0x5a],ax
.L4a3f70:
 mov    DWORD PTR [ebx+0x354],0x14
.L4a3f7a:
 mov    DWORD PTR [ebp-0x240],eax
.L4a3f80:
 mov    WORD PTR [ebp-0x23c],0x2
.L4a3f89:
 mov    DWORD PTR [ebp-0x23a],eax
.L4a3f8f:
 mov    DWORD PTR [ebp-0x236],0xf000c
.L4a3f99:
 mov    DWORD PTR [ebp-0x232],0x5
.L4a3fa3:
 mov    DWORD PTR [ebp-0x22e],0xf0010
.L4a3fad:
 mov    DWORD PTR [ebp-0x22a],0x105
.L4a3fb7:
 mov    DWORD PTR [ebp-0x226],0x40014
.L4a3fc1:
 mov    DWORD PTR [ebp-0x222],0xff000a
.L4a3fcb:
 mov    DWORD PTR [ebp-0x21e],0x110000
.L4a3fd5:
 mov    WORD PTR [ebp-0x21a],ax
.L4a3fdc:
 mov    DWORD PTR [ebx+0x358],0x18
.L4a3fe6:
 mov    DWORD PTR [ebp-0x100],eax
.L4a3fec:
 mov    WORD PTR [ebp-0xfc],0x2
.L4a3ff5:
 mov    DWORD PTR [ebp-0xfa],eax
.L4a3ffb:
 mov    DWORD PTR [ebp-0xf6],0x10000c
.L4a4005:
 mov    DWORD PTR [ebp-0xf2],0x3
.L4a400f:
 mov    DWORD PTR [ebp-0xee],0xf0014
.L4a4019:
 mov    DWORD PTR [ebp-0xea],0x5
.L4a4023:
 mov    DWORD PTR [ebp-0xe6],0x40018
.L4a402d:
 mov    DWORD PTR [ebp-0xe2],0xff000a
.L4a4037:
 mov    DWORD PTR [ebp-0xde],0x110000
.L4a4041:
 mov    WORD PTR [ebp-0xda],ax
.L4a4048:
 mov    DWORD PTR [ebx+0x35c],0x1c
.L4a4052:
 mov    DWORD PTR [ebp-0x2a0],eax
.L4a4058:
 mov    WORD PTR [ebp-0x29c],0x2
.L4a4061:
 mov    DWORD PTR [ebp-0x29a],eax
.L4a4067:
 mov    DWORD PTR [ebp-0x296],0x10000c
.L4a4071:
 mov    DWORD PTR [ebp-0x292],0x3
.L4a407b:
 mov    DWORD PTR [ebp-0x28e],0xf0014
.L4a4085:
 mov    DWORD PTR [ebp-0x28a],0x5
.L4a408f:
 mov    DWORD PTR [ebp-0x286],0xf0018
.L4a4099:
 mov    DWORD PTR [ebp-0x282],0x105
.L4a40a3:
 mov    DWORD PTR [ebp-0x27e],0x4001c
.L4a40ad:
 mov    DWORD PTR [ebp-0x27a],0xff000a
.L4a40b7:
 mov    DWORD PTR [ebp-0x276],0x110000
.L4a40c1:
 mov    WORD PTR [ebp-0x272],ax
.L4a40c8:
 mov    DWORD PTR [ebx+0x360],0x20
.L4a40d2:
 mov    DWORD PTR [ebp-0x1c8],eax
.L4a40d8:
 mov    WORD PTR [ebp-0x1c4],0x2
.L4a40e1:
 mov    DWORD PTR [ebp-0x1c2],eax
.L4a40e7:
 mov    DWORD PTR [ebp-0x1be],0x10000c
.L4a40f1:
 mov    DWORD PTR [ebp-0x1ba],0x3
.L4a40fb:
 mov    DWORD PTR [ebp-0x1b6],0xf0014
.L4a4105:
 mov    DWORD PTR [ebp-0x1b2],0x5
.L4a410f:
 mov    DWORD PTR [ebp-0x1ae],0x100018
.L4a4119:
 mov    DWORD PTR [ebp-0x1aa],0xff0006
.L4a4123:
 mov    DWORD PTR [ebp-0x1a6],0x110000
.L4a412d:
 mov    WORD PTR [ebp-0x1a2],ax
.L4a4134:
 mov    DWORD PTR [ebx+0x364],0x20
.L4a413e:
 mov    DWORD PTR [ebp-0x270],eax
.L4a4144:
 mov    WORD PTR [ebp-0x26c],0x2
.L4a414d:
 mov    DWORD PTR [ebp-0x26a],eax
.L4a4153:
 mov    DWORD PTR [ebp-0x266],0x10000c
.L4a415d:
 mov    DWORD PTR [ebp-0x262],0x3
.L4a4167:
 mov    DWORD PTR [ebp-0x25e],0xf0014
.L4a4171:
 mov    DWORD PTR [ebp-0x25a],0x5
.L4a417b:
 mov    DWORD PTR [ebp-0x256],0x40018
.L4a4185:
 mov    DWORD PTR [ebp-0x252],0xa
.L4a418f:
 mov    DWORD PTR [ebp-0x24e],0x10001c
.L4a4199:
 mov    DWORD PTR [ebp-0x24a],0xff0006
.L4a41a3:
 mov    DWORD PTR [ebp-0x246],0x110000
.L4a41ad:
 mov    WORD PTR [ebp-0x242],ax
.L4a41b4:
 mov    DWORD PTR [ebx+0x368],0x24
.L4a41be:
 mov    DWORD PTR [ebp-0x408],eax
.L4a41c4:
 mov    WORD PTR [ebp-0x404],0x2
.L4a41cd:
 mov    DWORD PTR [ebp-0x402],eax
.L4a41d3:
 mov    DWORD PTR [ebp-0x3fe],0x10000c
.L4a41dd:
 mov    DWORD PTR [ebp-0x3fa],0x3
.L4a41e7:
 mov    DWORD PTR [ebp-0x3f6],0xf0014
.L4a41f1:
 mov    DWORD PTR [ebp-0x3f2],0x5
.L4a41fb:
 mov    DWORD PTR [ebp-0x3ee],0xf0018
.L4a4205:
 mov    DWORD PTR [ebp-0x3ea],0x105
.L4a420f:
 mov    DWORD PTR [ebp-0x3e6],0x4001c
.L4a4219:
 mov    DWORD PTR [ebp-0x3e2],0xa
.L4a4223:
 mov    DWORD PTR [ebp-0x3de],0x100020
.L4a422d:
 mov    DWORD PTR [ebp-0x3da],0xff0006
.L4a4237:
 mov    DWORD PTR [ebp-0x3d6],0x110000
.L4a4241:
 mov    WORD PTR [ebp-0x3d2],ax
.L4a4248:
 mov    DWORD PTR [ebx+0x36c],0x28
.L4a4252:
 mov    DWORD PTR [ebp-0x300],eax
.L4a4258:
 mov    WORD PTR [ebp-0x2fc],0x2
.L4a4261:
 mov    DWORD PTR [ebp-0x2fa],eax
.L4a4267:
 mov    DWORD PTR [ebp-0x2f6],0x10000c
.L4a4271:
 mov    DWORD PTR [ebp-0x2f2],0x3
.L4a427b:
 mov    DWORD PTR [ebp-0x2ee],0xf0014
.L4a4285:
 mov    DWORD PTR [ebp-0x2ea],0x5
.L4a428f:
 mov    DWORD PTR [ebp-0x2e6],0xf0018
.L4a4299:
 mov    DWORD PTR [ebp-0x2e2],0x105
.L4a42a3:
 mov    DWORD PTR [ebp-0x2de],0x10001c
.L4a42ad:
 mov    DWORD PTR [ebp-0x2da],0xff0006
.L4a42b7:
 mov    DWORD PTR [ebp-0x2d6],0x110000
.L4a42c1:
 mov    WORD PTR [ebp-0x2d2],ax
.L4a42c8:
 mov    DWORD PTR [ebx+0x370],0x24
.L4a42d2:
 mov    DWORD PTR [ebp-0x360],eax
.L4a42d8:
 mov    WORD PTR [ebp-0x35c],0x2
.L4a42e1:
 mov    DWORD PTR [ebp-0x35a],eax
.L4a42e7:
 mov    DWORD PTR [ebp-0x356],0x10000c
.L4a42f1:
 mov    DWORD PTR [ebp-0x352],0x3
.L4a42fb:
 mov    DWORD PTR [ebp-0x34e],0xf0014
.L4a4305:
 mov    DWORD PTR [ebp-0x34a],0x5
.L4a430f:
 mov    DWORD PTR [ebp-0x346],0x50018
.L4a4319:
 mov    DWORD PTR [ebp-0x342],0x2
.L4a4323:
 mov    DWORD PTR [ebp-0x33e],0x10001c
.L4a432d:
 mov    DWORD PTR [ebp-0x33a],0xff0001
.L4a4337:
 mov    DWORD PTR [ebp-0x336],0x110000
.L4a4341:
 mov    WORD PTR [ebp-0x332],ax
.L4a4348:
 mov    DWORD PTR [ebx+0x374],0x24
.L4a4352:
 mov    DWORD PTR [ebp-0x398],eax
.L4a4358:
 mov    WORD PTR [ebp-0x394],0x2
.L4a4361:
 mov    DWORD PTR [ebp-0x392],eax
.L4a4367:
 mov    DWORD PTR [ebp-0x38e],0x10000c
.L4a4371:
 mov    DWORD PTR [ebp-0x38a],0x3
.L4a437b:
 mov    DWORD PTR [ebp-0x386],0xf0014
.L4a4385:
 mov    DWORD PTR [ebp-0x382],0x5
.L4a438f:
 mov    DWORD PTR [ebp-0x37e],0xf0018
.L4a4399:
 mov    DWORD PTR [ebp-0x37a],0x105
.L4a43a3:
 mov    DWORD PTR [ebp-0x376],0x5001c
.L4a43ad:
 mov    DWORD PTR [ebp-0x372],0x2
.L4a43b7:
 mov    DWORD PTR [ebp-0x36e],0x100020
.L4a43c1:
 mov    DWORD PTR [ebp-0x36a],0xff0001
.L4a43cb:
 mov    DWORD PTR [ebp-0x366],0x110000
.L4a43d5:
 mov    WORD PTR [ebp-0x362],ax
.L4a43dc:
 mov    DWORD PTR [ebx+0x378],0x28
.L4a43e6:
 mov    DWORD PTR [ebp-0x478],eax
.L4a43ec:
 mov    WORD PTR [ebp-0x474],0x2
.L4a43f5:
 mov    DWORD PTR [ebp-0x472],eax
.L4a43fb:
 mov    DWORD PTR [ebp-0x46e],0x10000c
.L4a4405:
 mov    DWORD PTR [ebp-0x46a],0x3
.L4a440f:
 mov    DWORD PTR [ebp-0x466],0xf0014
.L4a4419:
 mov    DWORD PTR [ebp-0x462],0x5
.L4a4423:
 mov    DWORD PTR [ebp-0x45e],0x100018
.L4a442d:
 mov    DWORD PTR [ebp-0x45a],0x6
.L4a4437:
 mov    DWORD PTR [ebp-0x456],0x50020
.L4a4441:
 mov    DWORD PTR [ebp-0x452],0x2
.L4a444b:
 mov    DWORD PTR [ebp-0x44e],0x100024
.L4a4455:
 mov    DWORD PTR [ebp-0x44a],0xff0001
.L4a445f:
 mov    DWORD PTR [ebp-0x446],0x110000
.L4a4469:
 mov    WORD PTR [ebp-0x442],ax
.L4a4470:
 mov    DWORD PTR [ebx+0x37c],0x2c
.L4a447a:
 mov    DWORD PTR [ebp-0x4b8],eax
.L4a4480:
 mov    WORD PTR [ebp-0x4b4],0x2
.L4a4489:
 mov    DWORD PTR [ebp-0x4b2],eax
.L4a448f:
 mov    DWORD PTR [ebp-0x4ae],0x10000c
.L4a4499:
 mov    DWORD PTR [ebp-0x4aa],0x3
.L4a44a3:
 mov    DWORD PTR [ebp-0x4a6],0xf0014
.L4a44ad:
 mov    DWORD PTR [ebp-0x4a2],0x5
.L4a44b7:
 mov    DWORD PTR [ebp-0x49e],0xf0018
.L4a44c1:
 mov    DWORD PTR [ebp-0x49a],0x105
.L4a44cb:
 mov    DWORD PTR [ebp-0x496],0xf001c
.L4a44d5:
 mov    DWORD PTR [ebp-0x492],0x205
.L4a44df:
 mov    DWORD PTR [ebp-0x48e],0x40020
.L4a44e9:
 mov    DWORD PTR [ebp-0x48a],0xa
.L4a44f3:
 mov    DWORD PTR [ebp-0x486],0x100024
.L4a44fd:
 mov    DWORD PTR [ebp-0x482],0xff0006
.L4a4507:
 mov    DWORD PTR [ebp-0x47e],0x110000
.L4a4511:
 mov    WORD PTR [ebp-0x47a],ax
.L4a4518:
 mov    DWORD PTR [ebx+0x380],0x2c
.L4a4522:
 mov    DWORD PTR [ebp-0x538],eax
.L4a4528:
 mov    WORD PTR [ebp-0x534],0x2
.L4a4531:
 mov    DWORD PTR [ebp-0x532],eax
.L4a4537:
 mov    DWORD PTR [ebp-0x52e],0x10000c
.L4a4541:
 mov    DWORD PTR [ebp-0x52a],0x3
.L4a454b:
 mov    DWORD PTR [ebp-0x526],0xf0014
.L4a4555:
 mov    DWORD PTR [ebp-0x522],0x5
.L4a455f:
 mov    DWORD PTR [ebp-0x51e],0xf0018
.L4a4569:
 mov    DWORD PTR [ebp-0x51a],0x105
.L4a4573:
 mov    DWORD PTR [ebp-0x516],0x10001c
.L4a457d:
 mov    DWORD PTR [ebp-0x512],0x6
.L4a4587:
 mov    DWORD PTR [ebp-0x50e],0x50024
.L4a4591:
 mov    DWORD PTR [ebp-0x50a],0x2
.L4a459b:
 mov    DWORD PTR [ebp-0x506],0x100028
.L4a45a5:
 mov    DWORD PTR [ebp-0x502],0xff0001
.L4a45af:
 mov    DWORD PTR [ebp-0x4fe],0x110000
.L4a45b9:
 mov    WORD PTR [ebp-0x4fa],ax
.L4a45c0:
 mov    DWORD PTR [ebx+0x384],0x30
.L4a45ca:
 mov    DWORD PTR [ebp-0x440],eax
.L4a45d0:
 mov    WORD PTR [ebp-0x43c],0x2
.L4a45d9:
 mov    DWORD PTR [ebp-0x43a],eax
.L4a45df:
 mov    DWORD PTR [ebp-0x436],0x10000c
.L4a45e9:
 mov    DWORD PTR [ebp-0x432],0x3
.L4a45f3:
 mov    DWORD PTR [ebp-0x42e],0xf0014
.L4a45fd:
 mov    DWORD PTR [ebp-0x42a],0x5
.L4a4607:
 mov    DWORD PTR [ebp-0x426],0x40018
.L4a4611:
 mov    DWORD PTR [ebp-0x422],0xa
.L4a461b:
 mov    DWORD PTR [ebp-0x41e],0x5001c
.L4a4625:
 mov    DWORD PTR [ebp-0x41a],0x2
.L4a462f:
 mov    DWORD PTR [ebp-0x416],0x100020
.L4a4639:
 mov    DWORD PTR [ebp-0x412],0xff0001
.L4a4643:
 mov    DWORD PTR [ebp-0x40e],0x110000
.L4a464d:
 mov    WORD PTR [ebp-0x40a],ax
.L4a4654:
 mov    DWORD PTR [ebx+0x388],0x28
.L4a465e:
 mov    DWORD PTR [ebp-0x4f8],eax
.L4a4664:
 mov    WORD PTR [ebp-0x4f4],0x2
.L4a466d:
 mov    DWORD PTR [ebp-0x4f2],eax
.L4a4673:
 mov    DWORD PTR [ebp-0x4ee],0x10000c
.L4a467d:
 mov    DWORD PTR [ebp-0x4ea],0x3
.L4a4687:
 mov    DWORD PTR [ebp-0x4e6],0xf0014
.L4a4691:
 mov    DWORD PTR [ebp-0x4e2],0x5
.L4a469b:
 mov    DWORD PTR [ebp-0x4de],0x40018
.L4a46a5:
 mov    DWORD PTR [ebp-0x4da],0xa
.L4a46af:
 mov    DWORD PTR [ebp-0x4d6],0x10001c
.L4a46b9:
 mov    DWORD PTR [ebp-0x4d2],0x6
.L4a46c3:
 mov    DWORD PTR [ebp-0x4ce],0x50024
.L4a46cd:
 mov    DWORD PTR [ebp-0x4ca],0x2
.L4a46d7:
 mov    DWORD PTR [ebp-0x4c6],0x100028
.L4a46e1:
 mov    DWORD PTR [ebp-0x4c2],0xff0001
.L4a46eb:
 mov    DWORD PTR [ebp-0x4be],0x110000
.L4a46f5:
 mov    WORD PTR [ebp-0x4ba],ax
.L4a46fc:
 mov    DWORD PTR [ebx+0x38c],0x30
.L4a4706:
 mov    DWORD PTR [ebp-0x3d0],eax
.L4a470c:
 mov    WORD PTR [ebp-0x3cc],0x2
.L4a4715:
 mov    DWORD PTR [ebp-0x3ca],eax
.L4a471b:
 mov    DWORD PTR [ebp-0x3c6],0x10000c
.L4a4725:
 mov    DWORD PTR [ebp-0x3c2],0x3
.L4a472f:
 mov    DWORD PTR [ebp-0x3be],0xf0014
.L4a4739:
 mov    DWORD PTR [ebp-0x3ba],0x5
.L4a4743:
 mov    DWORD PTR [ebp-0x3b6],0xf0018
.L4a474d:
 mov    DWORD PTR [ebp-0x3b2],0x105
.L4a4757:
 mov    DWORD PTR [ebp-0x3ae],0xf001c
.L4a4761:
 mov    DWORD PTR [ebp-0x3aa],0x205
.L4a476b:
 mov    DWORD PTR [ebp-0x3a6],0x40020
.L4a4775:
 mov    DWORD PTR [ebp-0x3a2],0xff000a
.L4a477f:
 mov    DWORD PTR [ebp-0x39e],0x110000
.L4a4789:
 mov    WORD PTR [ebp-0x39a],ax
.L4a4790:
 mov    DWORD PTR [ebx+0x390],0x24
.L4a479a:
 mov    DWORD PTR [ebp-0x5b8],eax
.L4a47a0:
 mov    WORD PTR [ebp-0x5b4],0x2
.L4a47a9:
 mov    DWORD PTR [ebp-0x5b2],eax
.L4a47af:
 mov    DWORD PTR [ebp-0x5ae],0x10000c
.L4a47b9:
 mov    DWORD PTR [ebp-0x5aa],0x3
.L4a47c3:
 mov    DWORD PTR [ebp-0x5a6],0xf0014
.L4a47cd:
 mov    DWORD PTR [ebp-0x5a2],0x5
.L4a47d7:
 mov    DWORD PTR [ebp-0x59e],0xf0018
.L4a47e1:
 mov    DWORD PTR [ebp-0x59a],0x105
.L4a47eb:
 mov    DWORD PTR [ebp-0x596],0xf001c
.L4a47f5:
 mov    DWORD PTR [ebp-0x592],0x205
.L4a47ff:
 mov    DWORD PTR [ebp-0x58e],0xf0020
.L4a4809:
 mov    DWORD PTR [ebp-0x58a],0x305
.L4a4813:
 mov    DWORD PTR [ebp-0x586],0x40024
.L4a481d:
 mov    DWORD PTR [ebp-0x582],0xff000a
.L4a4827:
 mov    DWORD PTR [ebp-0x57e],0x110000
.L4a4831:
 mov    WORD PTR [ebp-0x57a],ax
.L4a4838:
 mov    DWORD PTR [ebx+0x394],0x28
.L4a4842:
 mov    DWORD PTR [ebp-0x578],eax
.L4a4848:
 mov    WORD PTR [ebp-0x574],0x2
.L4a4851:
 mov    DWORD PTR [ebp-0x572],eax
.L4a4857:
 mov    DWORD PTR [ebp-0x56e],0x10000c
.L4a4861:
 mov    WORD PTR [ebp-0x53a],ax
.L4a4868:
 mov    DWORD PTR [ebp-0x2d0],eax
.L4a486e:
 mov    DWORD PTR [ebp-0x2ca],eax
.L4a4874:
 mov    WORD PTR [ebp-0x2a2],ax
.L4a487b:
 lea    eax,[ebp-0x680]
.L4a4881:
 mov    DWORD PTR [ebp-0x640],eax
.L4a4887:
 lea    eax,[ebp-0x40]
.L4a488a:
 mov    DWORD PTR [ebp-0x63c],eax
.L4a4890:
 lea    eax,[ebp-0xb8]
.L4a4896:
 mov    DWORD PTR [ebp-0x638],eax
.L4a489c:
 lea    eax,[ebp-0x178]
.L4a48a2:
 mov    DWORD PTR [ebp-0x634],eax
.L4a48a8:
 lea    eax,[ebp-0x58]
.L4a48ab:
 mov    DWORD PTR [ebp-0x630],eax
.L4a48b1:
 lea    eax,[ebp-0x98]
.L4a48b7:
 mov    DWORD PTR [ebp-0x62c],eax
.L4a48bd:
 lea    eax,[ebp-0x218]
.L4a48c3:
 mov    DWORD PTR [ebp-0x628],eax
.L4a48c9:
 lea    eax,[ebp-0x128]
.L4a48cf:
 mov    DWORD PTR [ebp-0x624],eax
.L4a48d5:
 lea    eax,[ebp-0x330]
.L4a48db:
 mov    DWORD PTR [ebp-0x620],eax
.L4a48e1:
 lea    eax,[ebp-0x150]
.L4a48e7:
 mov    DWORD PTR [ebp-0x61c],eax
.L4a48ed:
 lea    eax,[ebp-0x1a0]
.L4a48f3:
 mov    DWORD PTR [ebp-0x618],eax
.L4a48f9:
 lea    eax,[ebp-0x28]
.L4a48fc:
 mov    DWORD PTR [ebp-0x614],eax
.L4a4902:
 lea    eax,[ebp-0xd8]
.L4a4908:
 mov    DWORD PTR [ebp-0x610],eax
.L4a490e:
 lea    eax,[ebp-0x1f0]
.L4a4914:
 mov    DWORD PTR [ebp-0x60c],eax
.L4a491a:
 lea    eax,[ebp-0x78]
.L4a491d:
 mov    DWORD PTR [ebp-0x608],eax
.L4a4923:
 lea    eax,[ebp-0x240]
.L4a4929:
 mov    DWORD PTR [ebp-0x604],eax
.L4a492f:
 lea    eax,[ebp-0x100]
.L4a4935:
 mov    DWORD PTR [ebp-0x600],eax
.L4a493b:
 lea    eax,[ebp-0x2a0]
.L4a4941:
 mov    DWORD PTR [ebp-0x5fc],eax
.L4a4947:
 lea    eax,[ebp-0x1c8]
.L4a494d:
 mov    DWORD PTR [ebp-0x5f8],eax
.L4a4953:
 lea    eax,[ebp-0x270]
.L4a4959:
 mov    DWORD PTR [ebp-0x5f4],eax
.L4a495f:
 lea    eax,[ebp-0x408]
.L4a4965:
 mov    DWORD PTR [ebp-0x5f0],eax
.L4a496b:
 lea    eax,[ebp-0x300]
.L4a4971:
 mov    DWORD PTR [ebp-0x5ec],eax
.L4a4977:
 lea    eax,[ebp-0x360]
.L4a497d:
 mov    DWORD PTR [ebp-0x5e8],eax
.L4a4983:
 lea    eax,[ebp-0x398]
.L4a4989:
 mov    DWORD PTR [ebp-0x5e4],eax
.L4a498f:
 lea    eax,[ebp-0x478]
.L4a4995:
 mov    DWORD PTR [ebp-0x5e0],eax
.L4a499b:
 lea    eax,[ebp-0x4b8]
.L4a49a1:
 mov    DWORD PTR [ebp-0x5dc],eax
.L4a49a7:
 lea    eax,[ebp-0x538]
.L4a49ad:
 mov    DWORD PTR [ebp-0x56a],0x3
.L4a49b7:
 mov    DWORD PTR [ebp-0x566],0xf0014
.L4a49c1:
 mov    DWORD PTR [ebp-0x562],0x5
.L4a49cb:
 mov    DWORD PTR [ebp-0x55e],0xf0018
.L4a49d5:
 mov    DWORD PTR [ebp-0x55a],0x105
.L4a49df:
 mov    DWORD PTR [ebp-0x556],0x10001c
.L4a49e9:
 mov    DWORD PTR [ebp-0x552],0x6
.L4a49f3:
 mov    DWORD PTR [ebp-0x54e],0x20024
.L4a49fd:
 mov    DWORD PTR [ebp-0x54a],0x100
.L4a4a07:
 mov    DWORD PTR [ebp-0x546],0x20030
.L4a4a11:
 mov    DWORD PTR [ebp-0x542],0xff0200
.L4a4a1b:
 mov    DWORD PTR [ebp-0x53e],0x110000
.L4a4a25:
 mov    DWORD PTR [ebx+0x398],0x3c
.L4a4a2f:
 mov    WORD PTR [ebp-0x2cc],0x2
.L4a4a38:
 mov    DWORD PTR [ebp-0x2c6],0x10000c
.L4a4a42:
 mov    DWORD PTR [ebp-0x2c2],0x3
.L4a4a4c:
 mov    DWORD PTR [ebp-0x2be],0xf0014
.L4a4a56:
 mov    DWORD PTR [ebp-0x2ba],0x5
.L4a4a60:
 mov    DWORD PTR [ebp-0x2b6],0x20018
.L4a4a6a:
 mov    DWORD PTR [ebp-0x2b2],0x100
.L4a4a74:
 mov    DWORD PTR [ebp-0x2ae],0x20024
.L4a4a7e:
 mov    DWORD PTR [ebp-0x2aa],0xff0200
.L4a4a88:
 mov    DWORD PTR [ebp-0x2a6],0x110000
.L4a4a92:
 mov    DWORD PTR [ebx+0x39c],0x30
.L4a4a9c:
 mov    DWORD PTR [ebp-0x5d8],eax
.L4a4aa2:
 lea    edi,[ebx+0x298]
.L4a4aa8:
 lea    eax,[ebp-0x440]
.L4a4aae:
 {load} xor    esi,esi
.L4a4ab0:
 mov    DWORD PTR [ebp-0x5d4],eax
.L4a4ab6:
 lea    eax,[ebp-0x4f8]
.L4a4abc:
 mov    DWORD PTR [ebp-0x5d0],eax
.L4a4ac2:
 lea    eax,[ebp-0x3d0]
.L4a4ac8:
 mov    DWORD PTR [ebp-0x5cc],eax
.L4a4ace:
 lea    eax,[ebp-0x5b8]
.L4a4ad4:
 mov    DWORD PTR [ebp-0x5c8],eax
.L4a4ada:
 lea    eax,[ebp-0x578]
.L4a4ae0:
 mov    DWORD PTR [ebp-0x5c4],eax
.L4a4ae6:
 lea    eax,[ebp-0x2d0]
.L4a4aec:
 mov    DWORD PTR [ebp-0x5c0],eax
.L4a4af2:
 mov    eax,DWORD PTR [ebx+0x78]
.L4a4af5:
 push   edi
.L4a4af6:
 push   DWORD PTR [ebp+esi*4-0x640]
.L4a4afd:
 mov    ecx,DWORD PTR [eax]
.L4a4aff:
 push   eax
.L4a4b00:
 call   DWORD PTR [ecx+0x158]
.L4a4b06:
 test   eax,eax
.L4a4b08:
 js .L4a4b1a
.L4a4b0a:
 inc    esi
.L4a4b0b:
 add    edi,0x4
.L4a4b0e:
 cmp    esi,0x21
.L4a4b11:
 jb .L4a4af2
.L4a4b13:
 mov    al,0x1
.L4a4b15:
 jmp .L4a4c27
.L4a4b1a:
 {load} mov    edx,eax
.L4a4b1c:
 lea    ecx,[ebp-0x670]
.L4a4b22:
 call target_44f380
.L4a4b27:
 push   eax
.L4a4b28:
 lea    eax,[ebp-0x658]
.L4a4b2e:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a4b35:
 push   0x9fc580
.L4a4b3a:
 push   eax
.L4a4b3b:
 call target_4b2d20
.L4a4b40:
 {load} mov    esi,eax
.L4a4b42:
 lea    edi,[ebx+0x54]
.L4a4b45:
 mov    eax,[0x92d4b0]
.L4a4b4a:
 add    esp,0xc
.L4a4b4d:
 {load} cmp    edi,esi
.L4a4b4f:
 je .L4a4bb5
.L4a4b51:
 cmp    DWORD PTR [edi+0x14],0x10
.L4a4b55:
 jb .L4a4b5e
.L4a4b57:
 push   DWORD PTR [edi]
.L4a4b59:
 call   eax
.L4a4b5b:
 add    esp,0x4
.L4a4b5e:
 mov    DWORD PTR [edi+0x14],0xf
.L4a4b65:
 mov    DWORD PTR [edi+0x10],0x0
.L4a4b6c:
 mov    BYTE PTR [edi],0x0
.L4a4b6f:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a4b73:
 jae .L4a4b89
.L4a4b75:
 mov    eax,DWORD PTR [esi+0x10]
.L4a4b78:
 inc    eax
.L4a4b79:
 je .L4a4b93
.L4a4b7b:
 push   eax
.L4a4b7c:
 push   esi
.L4a4b7d:
 push   edi
.L4a4b7e:
 call   DWORD PTR [0x92d4a4]
.L4a4b84:
 add    esp,0xc
.L4a4b87:
 jmp .L4a4b93
.L4a4b89:
 mov    eax,DWORD PTR [esi]
.L4a4b8b:
 mov    DWORD PTR [edi],eax
.L4a4b8d:
 mov    DWORD PTR [esi],0x0
.L4a4b93:
 mov    eax,DWORD PTR [esi+0x10]
.L4a4b96:
 mov    DWORD PTR [edi+0x10],eax
.L4a4b99:
 mov    eax,DWORD PTR [esi+0x14]
.L4a4b9c:
 mov    DWORD PTR [edi+0x14],eax
.L4a4b9f:
 mov    eax,[0x92d4b0]
.L4a4ba4:
 mov    DWORD PTR [esi+0x14],0xf
.L4a4bab:
 mov    DWORD PTR [esi+0x10],0x0
.L4a4bb2:
 mov    BYTE PTR [esi],0x0
.L4a4bb5:
 cmp    DWORD PTR [ebp-0x644],0x10
.L4a4bbc:
 jb .L4a4bc9
.L4a4bbe:
 push   DWORD PTR [ebp-0x658]
.L4a4bc4:
 call   eax
.L4a4bc6:
 add    esp,0x4
.L4a4bc9:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a4bd0:
 cmp    DWORD PTR [ebp-0x65c],0x10
.L4a4bd7:
 mov    DWORD PTR [ebp-0x644],0xf
.L4a4be1:
 mov    DWORD PTR [ebp-0x648],0x0
.L4a4beb:
 mov    BYTE PTR [ebp-0x658],0x0
.L4a4bf2:
 jb .L4a4c03
.L4a4bf4:
 push   DWORD PTR [ebp-0x670]
.L4a4bfa:
 call   DWORD PTR [0x92d4b0]
.L4a4c00:
 add    esp,0x4
.L4a4c03:
 {load} mov    ecx,ebx
.L4a4c05:
 mov    DWORD PTR [ebp-0x65c],0xf
.L4a4c0f:
 mov    DWORD PTR [ebp-0x660],0x0
.L4a4c19:
 mov    BYTE PTR [ebp-0x670],0x0
.L4a4c20:
 call target_4a5c40
.L4a4c25:
 {load} xor    al,al
.L4a4c27:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a4c2a:
 mov    DWORD PTR fs:0x0,ecx
.L4a4c31:
 pop    ecx
.L4a4c32:
 pop    edi
.L4a4c33:
 pop    esi
.L4a4c34:
 pop    ebx
.L4a4c35:
 mov    ecx,DWORD PTR [ebp-0x10]
.L4a4c38:
 {load} xor    ecx,ebp
.L4a4c3a:
 call target_8f8958
.L4a4c3f:
 {load} mov    esp,ebp
.L4a4c41:
 pop    ebp
.L4a4c42:
 ret
.equ target_44f380,0x44f380
.equ target_4a5c40,0x4a5c40
.equ target_4b2d20,0x4b2d20
.equ target_8f8958,0x8f8958
