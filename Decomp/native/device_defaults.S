# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a5e80..0x4a632a
.intel_syntax noprefix
.code32
.text
.global original_device_defaults
original_device_defaults:
.L4a5e80:
 push   ebp
.L4a5e81:
 {load} mov    ebp,esp
.L4a5e83:
 push   ecx
.L4a5e84:
 mov    DWORD PTR [ecx+0x4f8],0x0
.L4a5e8e:
 mov    DWORD PTR [ecx+0x4fc],0x3
.L4a5e98:
 mov    DWORD PTR [ecx+0x500],0x2
.L4a5ea2:
 mov    DWORD PTR [ecx+0x514],0x1
.L4a5eac:
 mov    DWORD PTR [ecx+0x518],0x0
.L4a5eb6:
 mov    DWORD PTR [ecx+0x51c],0x1
.L4a5ec0:
 mov    DWORD PTR [ecx+0x528],0x2
.L4a5eca:
 mov    DWORD PTR [ecx+0x52c],0x1
.L4a5ed4:
 mov    DWORD PTR [ecx+0x534],0x3
.L4a5ede:
 mov    DWORD PTR [ecx+0x538],0x4
.L4a5ee8:
 mov    DWORD PTR [ecx+0x53c],0x0
.L4a5ef2:
 mov    DWORD PTR [ecx+0x540],0x8
.L4a5efc:
 mov    DWORD PTR [ecx+0x544],0x0
.L4a5f06:
 mov    DWORD PTR [ecx+0x548],0x0
.L4a5f10:
 mov    DWORD PTR [ecx+0x54c],0x0
.L4a5f1a:
 mov    DWORD PTR [ecx+0x550],0x0
.L4a5f24:
 mov    DWORD PTR [ecx+0x564],0x0
.L4a5f2e:
 mov    DWORD PTR [ecx+0x568],0x0
.L4a5f38:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a5f3f:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a5f42:
 mov    DWORD PTR [ecx+0x56c],eax
.L4a5f48:
 mov    DWORD PTR [ecx+0x59c],0x0
.L4a5f52:
 mov    DWORD PTR [ecx+0x5ac],0x0
.L4a5f5c:
 mov    DWORD PTR [ecx+0x5b0],0x1
.L4a5f66:
 mov    DWORD PTR [ecx+0x5b4],0x1
.L4a5f70:
 mov    DWORD PTR [ecx+0x5b8],0x1
.L4a5f7a:
 mov    DWORD PTR [ecx+0x5bc],0x8
.L4a5f84:
 mov    DWORD PTR [ecx+0x5c0],0x0
.L4a5f8e:
 mov    DWORD PTR [ecx+0x5c4],0xffffffff
.L4a5f98:
 mov    DWORD PTR [ecx+0x5c8],0xffffffff
.L4a5fa2:
 mov    DWORD PTR [ecx+0x5cc],0xffffffff
.L4a5fac:
 mov    DWORD PTR [ebp-0x4],0x3f800000
.L4a5fb3:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a5fb6:
 mov    DWORD PTR [ecx+0x570],eax
.L4a5fbc:
 mov    DWORD PTR [ecx+0x574],eax
.L4a5fc2:
 mov    DWORD PTR [ecx+0x6dc],0x0
.L4a5fcc:
 mov    DWORD PTR [ecx+0x6e0],0x0
.L4a5fd6:
 mov    DWORD PTR [ecx+0x6e4],0x0
.L4a5fe0:
 mov    DWORD PTR [ecx+0x6e8],0x0
.L4a5fea:
 mov    DWORD PTR [ecx+0x6ec],0x0
.L4a5ff4:
 mov    DWORD PTR [ecx+0x6f0],0x0
.L4a5ffe:
 mov    DWORD PTR [ecx+0x6f4],0x0
.L4a6008:
 mov    DWORD PTR [ecx+0x6f8],0x0
.L4a6012:
 mov    DWORD PTR [ebp-0x4],0x42800000
.L4a6019:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a601c:
 mov    DWORD PTR [ecx+0x744],eax
.L4a6022:
 mov    DWORD PTR [ebp-0x4],0x3f800000
.L4a6029:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a602c:
 mov    DWORD PTR [ecx+0x748],eax
.L4a6032:
 mov    DWORD PTR [ecx+0x754],eax
.L4a6038:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a603f:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a6042:
 mov    DWORD PTR [ebp-0x4],0x42800000
.L4a6049:
 mov    DWORD PTR [ecx+0x758],eax
.L4a604f:
 mov    DWORD PTR [ecx+0x75c],eax
.L4a6055:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a6058:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a605f:
 mov    DWORD PTR [ecx+0x774],eax
.L4a6065:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a6068:
 mov    DWORD PTR [ecx+0x6fc],0x1
.L4a6072:
 mov    DWORD PTR [ecx+0x700],0x1
.L4a607c:
 mov    DWORD PTR [ecx+0x708],0x0
.L4a6086:
 mov    DWORD PTR [ecx+0x70c],0x0
.L4a6090:
 mov    DWORD PTR [ecx+0x710],0x1
.L4a609a:
 mov    DWORD PTR [ecx+0x714],0x1
.L4a60a4:
 mov    DWORD PTR [ecx+0x718],0x0
.L4a60ae:
 mov    DWORD PTR [ecx+0x720],0x1
.L4a60b8:
 mov    DWORD PTR [ecx+0x724],0x2
.L4a60c2:
 mov    DWORD PTR [ecx+0x728],0x0
.L4a60cc:
 mov    DWORD PTR [ecx+0x72c],0x0
.L4a60d6:
 mov    DWORD PTR [ecx+0x738],0x0
.L4a60e0:
 mov    DWORD PTR [ecx+0x73c],0x0
.L4a60ea:
 mov    DWORD PTR [ecx+0x74c],0x0
.L4a60f4:
 mov    DWORD PTR [ecx+0x750],0x0
.L4a60fe:
 mov    DWORD PTR [ecx+0x760],0x1
.L4a6108:
 mov    DWORD PTR [ecx+0x764],0xffffffff
.L4a6112:
 mov    DWORD PTR [ecx+0x768],0x0
.L4a611c:
 mov    DWORD PTR [ecx+0x770],0x0
.L4a6126:
 mov    DWORD PTR [ecx+0x778],0x0
.L4a6130:
 mov    DWORD PTR [ecx+0x77c],0xf
.L4a613a:
 mov    DWORD PTR [ecx+0x784],eax
.L4a6140:
 mov    DWORD PTR [ebp-0x4],0x3f800000
.L4a6147:
 mov    edx,0x10
.L4a614c:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a614f:
 mov    DWORD PTR [ecx+0x7a4],eax
.L4a6155:
 mov    DWORD PTR [ecx+0x7a8],eax
.L4a615b:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a6162:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a6165:
 mov    DWORD PTR [ecx+0x7ac],eax
.L4a616b:
 mov    DWORD PTR [ecx+0x7b0],eax
.L4a6171:
 mov    DWORD PTR [ebp-0x4],0x3f800000
.L4a6178:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a617b:
 mov    DWORD PTR [ecx+0x7b4],eax
.L4a6181:
 mov    DWORD PTR [ecx+0x788],0x1
.L4a618b:
 mov    DWORD PTR [ecx+0x78c],0x3
.L4a6195:
 mov    DWORD PTR [ecx+0x790],0x1
.L4a619f:
 mov    DWORD PTR [ecx+0x794],0x0
.L4a61a9:
 mov    DWORD PTR [ecx+0x798],0x0
.L4a61b3:
 mov    DWORD PTR [ecx+0x79c],0x0
.L4a61bd:
 mov    DWORD PTR [ecx+0x7bc],0x0
.L4a61c7:
 mov    DWORD PTR [ecx+0x7c0],0x0
.L4a61d1:
 mov    DWORD PTR [ecx+0x7c4],0x1
.L4a61db:
 mov    DWORD PTR [ecx+0x7c8],0x1
.L4a61e5:
 mov    DWORD PTR [ecx+0x7cc],0x1
.L4a61ef:
 mov    DWORD PTR [ecx+0x7d0],0x8
.L4a61f9:
 mov    DWORD PTR [ecx+0x7d4],0xf
.L4a6203:
 mov    DWORD PTR [ecx+0x7d8],0xf
.L4a620d:
 mov    DWORD PTR [ecx+0x7dc],0xf
.L4a6217:
 mov    DWORD PTR [ecx+0x7e0],0xffffffff
.L4a6221:
 mov    DWORD PTR [ecx+0x7e4],0x0
.L4a622b:
 mov    DWORD PTR [ecx+0x7f4],0x0
.L4a6235:
 mov    DWORD PTR [ecx+0x7f8],0x0
.L4a623f:
 mov    DWORD PTR [ecx+0x7fc],0x0
.L4a6249:
 mov    DWORD PTR [ecx+0x800],0x0
.L4a6253:
 mov    DWORD PTR [ecx+0x804],0x0
.L4a625d:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a6264:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a6267:
 mov    DWORD PTR [ecx+0x7b8],eax
.L4a626d:
 mov    DWORD PTR [ecx+0x7e8],eax
.L4a6273:
 lea    eax,[ecx+0x864]
.L4a6279:
 mov    DWORD PTR [ecx+0x808],0x0
.L4a6283:
 mov    DWORD PTR [ecx+0x80c],0x0
.L4a628d:
 mov    DWORD PTR [ecx+0x810],0x0
.L4a6297:
 mov    DWORD PTR [ecx+0x814],0x0
.L4a62a1:
 mov    DWORD PTR [ecx+0x818],0x2
.L4a62ab:
 mov    DWORD PTR [ecx+0x81c],0x1
.L4a62b5:
 mov    DWORD PTR [ecx+0x820],0x1
.L4a62bf:
 nop
.L4a62c0:
 mov    DWORD PTR [eax-0x40],0x1
.L4a62c7:
 lea    eax,[eax+0x4]
.L4a62ca:
 mov    DWORD PTR [eax-0x4],0x1
.L4a62d1:
 mov    DWORD PTR [eax+0x3c],0x0
.L4a62d8:
 mov    DWORD PTR [eax+0x7c],0x1
.L4a62df:
 mov    DWORD PTR [eax+0xbc],0x1
.L4a62e9:
 mov    DWORD PTR [eax+0xfc],0x1
.L4a62f3:
 mov    DWORD PTR [eax+0x144],0x0
.L4a62fd:
 mov    DWORD PTR [eax+0x184],0x0
.L4a6307:
 dec    edx
.L4a6308:
 jne .L4a62c0
.L4a630a:
 mov    DWORD PTR [ecx+0xa68],edx
.L4a6310:
 mov    DWORD PTR [ecx+0xa64],0xffffffff
.L4a631a:
 mov    DWORD PTR [ecx+0xa6c],edx
.L4a6320:
 mov    DWORD PTR [ecx+0xa70],edx
.L4a6326:
 {load} mov    esp,ebp
.L4a6328:
 pop    ebp
.L4a6329:
 ret
