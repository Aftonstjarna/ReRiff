# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a0af0..0x4a1861
.intel_syntax noprefix
.code32
.text
.global original_application_frame
original_application_frame:
.L4a0af0:
 push   ebp
.L4a0af1:
 {load} mov    ebp,esp
.L4a0af3:
 and    esp,0xffffffc0
.L4a0af6:
 sub    esp,0xf4
.L4a0afc:
 mov    eax,[0xbaabc0]
.L4a0b01:
 {load} xor    eax,esp
.L4a0b03:
 mov    DWORD PTR [esp+0xf0],eax
.L4a0b0a:
 push   ebx
.L4a0b0b:
 push   esi
.L4a0b0c:
 {load} mov    esi,ecx
.L4a0b0e:
 push   edi
.L4a0b0f:
 mov    DWORD PTR [esp+0x50],esi
.L4a0b13:
 cmp    BYTE PTR [esi+0xba0],0x0
.L4a0b1a:
 jne .L4a184c
.L4a0b20:
 mov    eax,DWORD PTR [esi+0x488]
.L4a0b26:
 push   0xffffffff
.L4a0b28:
 push   DWORD PTR [esi+eax*4+0x458]
.L4a0b2f:
 call   DWORD PTR [0x92d124]
.L4a0b35:
 mov    eax,DWORD PTR [esi+0xba4]
.L4a0b3b:
 test   eax,eax
.L4a0b3d:
 je .L4a0b69
.L4a0b3f:
 push   eax
.L4a0b40:
 {load} mov    ecx,esi
.L4a0b42:
 call target_4a1900
.L4a0b47:
 mov    BYTE PTR [esi+0xba8],al
.L4a0b4d:
 mov    eax,DWORD PTR [esi+0x488]
.L4a0b53:
 mov    DWORD PTR [esi+0xba4],0x0
.L4a0b5d:
 push   DWORD PTR [esi+eax*4+0x460]
.L4a0b64:
 jmp .L4a1846
.L4a0b69:
 mov    eax,DWORD PTR [esi+0x488]
.L4a0b6f:
 mov    edi,DWORD PTR [esi+eax*4+0x468]
.L4a0b76:
 cmp    BYTE PTR [edi+0x91],0x0
.L4a0b7d:
 je .L4a0b8c
.L4a0b7f:
 mov    BYTE PTR [esi+eax*1+0x454],0x1
.L4a0b87:
 jmp .L4a1828
.L4a0b8c:
 mov    al,BYTE PTR [edi+0x92]
.L4a0b92:
 cmp    BYTE PTR [esi+0x4b8],al
.L4a0b98:
 je .L4a0bb3
.L4a0b9a:
 push   DWORD PTR [esi+0x490]
.L4a0ba0:
 {load} mov    ecx,esi
.L4a0ba2:
 mov    BYTE PTR [esi+0x4b8],al
.L4a0ba8:
 push   DWORD PTR [esi+0x48c]
.L4a0bae:
 call target_4a22e0
.L4a0bb3:
 movss  xmm0,DWORD PTR [esi+0xb7c]
.L4a0bbb:
 lea    ecx,[esi+0xabc]
.L4a0bc1:
 subss  xmm0,DWORD PTR [esi+0xb90]
.L4a0bc9:
 mov    al,BYTE PTR [edi+0x93]
.L4a0bcf:
 mov    edx,0xb
.L4a0bd4:
 mov    BYTE PTR [esi+0x4b9],al
.L4a0bda:
 movss  DWORD PTR [esi+0xb7c],xmm0
.L4a0be2:
 xorps  xmm0,xmm0
.L4a0be5:
 comiss xmm0,DWORD PTR [esi+0xb7c]
.L4a0bec:
 jb .L4a0bf9
.L4a0bee:
 mov    eax,DWORD PTR [ecx]
.L4a0bf0:
 mov    DWORD PTR [ecx-0x4],eax
.L4a0bf3:
 mov    DWORD PTR [ecx],0x0
.L4a0bf9:
 mov    eax,DWORD PTR [ecx-0x8]
.L4a0bfc:
 cmp    eax,DWORD PTR [ecx]
.L4a0bfe:
 jbe .L4a0c02
.L4a0c00:
 mov    DWORD PTR [ecx],eax
.L4a0c02:
 mov    DWORD PTR [ecx-0xc],0x0
.L4a0c09:
 mov    DWORD PTR [ecx-0x8],0x0
.L4a0c10:
 add    ecx,0x10
.L4a0c13:
 dec    edx
.L4a0c14:
 jne .L4a0be5
.L4a0c16:
 comiss xmm0,DWORD PTR [esi+0xb7c]
.L4a0c1d:
 jb .L4a0c29
.L4a0c1f:
 mov    DWORD PTR [esi+0xb7c],0x44fa0000
.L4a0c29:
 mov    BYTE PTR [esi+0xb80],0x0
.L4a0c30:
 mov    DWORD PTR [esi+0xb60],0x0
.L4a0c3a:
 mov    DWORD PTR [esi+0xb64],0x0
.L4a0c44:
 mov    DWORD PTR [esi+0xb68],0x0
.L4a0c4e:
 mov    DWORD PTR [esi+0xb6c],0x0
.L4a0c58:
 mov    DWORD PTR [esi+0xb70],0x0
.L4a0c62:
 mov    DWORD PTR [esi+0xb74],0x0
.L4a0c6c:
 mov    DWORD PTR [esi+0xb78],0x0
.L4a0c76:
 mov    DWORD PTR [esi+0x220],0x0
.L4a0c80:
 mov    DWORD PTR [esi+0x224],0x0
.L4a0c8a:
 cmp    BYTE PTR [esi+0xa74],0x0
.L4a0c91:
 je .L4a0ca9
.L4a0c93:
 mov    ebx,DWORD PTR [0xbfcc28]
.L4a0c99:
 call   DWORD PTR [0x92d558]
.L4a0c9f:
 cmp    eax,DWORD PTR [ebx+0x4]
.L4a0ca2:
 je .L4a0ca9
.L4a0ca4:
 cmp    eax,DWORD PTR [ebx+0x8]
.L4a0ca7:
 jne .L4a0cbc
.L4a0ca9:
 mov    eax,DWORD PTR [esi+0x78]
.L4a0cac:
 push   eax
.L4a0cad:
 mov    ecx,DWORD PTR [eax]
.L4a0caf:
 call   DWORD PTR [ecx+0xc]
.L4a0cb2:
 {load} mov    ebx,eax
.L4a0cb4:
 cmp    ebx,0x88760868
.L4a0cba:
 jne .L4a0cd9
.L4a0cbc:
 cmp    BYTE PTR [esi+0xa74],0x0
.L4a0cc3:
 jne .L4a0ccc
.L4a0cc5:
 {load} mov    ecx,esi
.L4a0cc7:
 call target_4a6330
.L4a0ccc:
 push   0x32
.L4a0cce:
 call   DWORD PTR [0x92d0d0]
.L4a0cd4:
 jmp .L4a181a
.L4a0cd9:
 cmp    ebx,0x88760869
.L4a0cdf:
 jne .L4a0d12
.L4a0ce1:
 cmp    BYTE PTR [esi+0xa74],0x0
.L4a0ce8:
 jne .L4a0cf1
.L4a0cea:
 {load} mov    ecx,esi
.L4a0cec:
 call target_4a6330
.L4a0cf1:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a0cf4:
 lea    eax,[esi+0x1b8]
.L4a0cfa:
 push   eax
.L4a0cfb:
 push   ecx
.L4a0cfc:
 mov    edx,DWORD PTR [ecx]
.L4a0cfe:
 call   DWORD PTR [edx+0x40]
.L4a0d01:
 {load} mov    ebx,eax
.L4a0d03:
 test   ebx,ebx
.L4a0d05:
 js .L4a181a
.L4a0d0b:
 {load} mov    ecx,esi
.L4a0d0d:
 call target_4a6560
.L4a0d12:
 test   ebx,ebx
.L4a0d14:
 js .L4a181a
.L4a0d1a:
 lea    ecx,[esi+0x3a0]
.L4a0d20:
 call target_542f80
.L4a0d25:
 mov    eax,DWORD PTR [edi+0x4]
.L4a0d28:
 {load} xor    ebx,ebx
.L4a0d2a:
 mov    ecx,DWORD PTR [edi]
.L4a0d2c:
 {load} sub    eax,ecx
.L4a0d2e:
 movd   xmm4,DWORD PTR [esi+0x49c]
.L4a0d36:
 movd   xmm5,DWORD PTR [esi+0x4a0]
.L4a0d3e:
 movss  xmm3,DWORD PTR [0xa06558]
.L4a0d46:
 sar    eax,0x2
.L4a0d49:
 cvtdq2ps xmm4,xmm4
.L4a0d4c:
 cvtdq2ps xmm5,xmm5
.L4a0d4f:
 test   eax,eax
.L4a0d51:
 je .L4a0f00
.L4a0d57:
 mov    esi,DWORD PTR [0xbfcd0c]
.L4a0d5d:
 {disp8} lea    ecx,[ecx+0x0]
.L4a0d60:
 mov    ecx,DWORD PTR [ecx+ebx*4]
.L4a0d63:
 mov    eax,DWORD PTR [ecx+0x5dc]
.L4a0d69:
 test   eax,eax
.L4a0d6b:
 jne .L4a0e0a
.L4a0d71:
 movss  xmm0,DWORD PTR [ecx+0x78]
.L4a0d76:
 addss  xmm0,xmm4
.L4a0d7a:
 movss  DWORD PTR [ecx+0x78],xmm0
.L4a0d7f:
 movss  xmm0,DWORD PTR [ecx+0x7c]
.L4a0d84:
 addss  xmm0,xmm5
.L4a0d88:
 movss  DWORD PTR [ecx+0x7c],xmm0
.L4a0d8d:
 movss  xmm0,DWORD PTR [ecx+0x80]
.L4a0d95:
 addss  xmm0,xmm4
.L4a0d99:
 movss  DWORD PTR [ecx+0x80],xmm0
.L4a0da1:
 movss  xmm0,DWORD PTR [ecx+0x84]
.L4a0da9:
 addss  xmm0,xmm5
.L4a0dad:
 movss  DWORD PTR [ecx+0x84],xmm0
.L4a0db5:
 movss  xmm0,DWORD PTR [ecx+0x5bc]
.L4a0dbd:
 addss  xmm0,xmm4
.L4a0dc1:
 movss  DWORD PTR [ecx+0x5bc],xmm0
.L4a0dc9:
 movss  xmm0,DWORD PTR [ecx+0x5c0]
.L4a0dd1:
 addss  xmm0,xmm4
.L4a0dd5:
 movss  DWORD PTR [ecx+0x5c0],xmm0
.L4a0ddd:
 movss  xmm0,DWORD PTR [ecx+0x5c4]
.L4a0de5:
 addss  xmm0,xmm5
.L4a0de9:
 movss  DWORD PTR [ecx+0x5c4],xmm0
.L4a0df1:
 movss  xmm0,DWORD PTR [ecx+0x5c8]
.L4a0df9:
 addss  xmm0,xmm5
.L4a0dfd:
 movss  DWORD PTR [ecx+0x5c8],xmm0
.L4a0e05:
 jmp .L4a0ee9
.L4a0e0a:
 cmp    eax,0x3
.L4a0e0d:
 jne .L4a0ee9
.L4a0e13:
 mov    edx,DWORD PTR [ecx+0x5d4]
.L4a0e19:
 cmp    edx,0xc
.L4a0e1c:
 jl .L4a0e23
.L4a0e1e:
 movaps xmm2,xmm3
.L4a0e21:
 jmp .L4a0e50
.L4a0e23:
 mov    eax,DWORD PTR [esi+0x1c4]
.L4a0e29:
 sub    eax,DWORD PTR [esi+0x1c0]
.L4a0e2f:
 mov    edx,DWORD PTR [esi+edx*4+0x1cc]
.L4a0e36:
 sar    eax,0x4
.L4a0e39:
 {load} cmp    edx,eax
.L4a0e3b:
 jb .L4a0e42
.L4a0e3d:
 movaps xmm2,xmm3
.L4a0e40:
 jmp .L4a0e50
.L4a0e42:
 mov    eax,DWORD PTR [esi+0x1c0]
.L4a0e48:
 {load} add    edx,edx
.L4a0e4a:
 movss  xmm2,DWORD PTR [eax+edx*8+0x1c]
.L4a0e50:
 movss  xmm0,DWORD PTR [ecx+0x78]
.L4a0e55:
 shufps xmm2,xmm2,0x0
.L4a0e59:
 mulss  xmm0,xmm2
.L4a0e5d:
 movss  DWORD PTR [ecx+0x78],xmm0
.L4a0e62:
 movaps xmm0,xmm2
.L4a0e65:
 mulss  xmm0,DWORD PTR [ecx+0x7c]
.L4a0e6a:
 movss  DWORD PTR [ecx+0x7c],xmm0
.L4a0e6f:
 movss  xmm0,DWORD PTR [ecx+0x88]
.L4a0e77:
 mulss  xmm0,xmm2
.L4a0e7b:
 movss  DWORD PTR [ecx+0x88],xmm0
.L4a0e83:
 movaps xmm0,xmm2
.L4a0e86:
 mulss  xmm0,DWORD PTR [ecx+0x8c]
.L4a0e8e:
 movss  DWORD PTR [ecx+0x8c],xmm0
.L4a0e96:
 movss  xmm0,DWORD PTR [ecx+0x88]
.L4a0e9e:
 addss  xmm0,DWORD PTR [ecx+0x78]
.L4a0ea3:
 movss  xmm1,DWORD PTR [ecx+0x8c]
.L4a0eab:
 addss  xmm1,DWORD PTR [ecx+0x7c]
.L4a0eb0:
 subss  xmm0,xmm3
.L4a0eb4:
 subss  xmm1,xmm3
.L4a0eb8:
 movss  DWORD PTR [esp+0x64],xmm0
.L4a0ebe:
 movups xmm0,XMMWORD PTR [ecx+0x5bc]
.L4a0ec5:
 mov    eax,DWORD PTR [esp+0x64]
.L4a0ec9:
 movss  DWORD PTR [esp+0x68],xmm1
.L4a0ecf:
 mulps  xmm0,xmm2
.L4a0ed2:
 mov    DWORD PTR [ecx+0x80],eax
.L4a0ed8:
 mov    eax,DWORD PTR [esp+0x68]
.L4a0edc:
 mov    DWORD PTR [ecx+0x84],eax
.L4a0ee2:
 movups XMMWORD PTR [ecx+0x5bc],xmm0
.L4a0ee9:
 mov    eax,DWORD PTR [edi+0x4]
.L4a0eec:
 inc    ebx
.L4a0eed:
 mov    ecx,DWORD PTR [edi]
.L4a0eef:
 {load} sub    eax,ecx
.L4a0ef1:
 sar    eax,0x2
.L4a0ef4:
 {load} cmp    ebx,eax
.L4a0ef6:
 jb .L4a0d60
.L4a0efc:
 mov    esi,DWORD PTR [esp+0x50]
.L4a0f00:
 mov    eax,DWORD PTR [edi+0x80]
.L4a0f06:
 mov    DWORD PTR [esi+0x4c8],eax
.L4a0f0c:
 movd   xmm0,eax
.L4a0f10:
 cvtdq2pd xmm0,xmm0
.L4a0f14:
 shr    eax,0x1f
.L4a0f17:
 addsd  xmm0,QWORD PTR [eax*8+0xa071b0]
.L4a0f20:
 cvtpd2ps xmm0,xmm0
.L4a0f24:
 mulss  xmm0,DWORD PTR [esi+0x4b4]
.L4a0f2c:
 mov    eax,DWORD PTR [esi+0x488]
.L4a0f32:
 mulss  xmm0,DWORD PTR [0xa0644c]
.L4a0f3a:
 lea    eax,[eax+eax*2]
.L4a0f3d:
 addss  xmm0,DWORD PTR [esi+0x4cc]
.L4a0f45:
 movss  DWORD PTR [esi+0x4cc],xmm0
.L4a0f4d:
 mov    ecx,DWORD PTR [esi+eax*4+0x470]
.L4a0f54:
 test   ecx,ecx
.L4a0f56:
 je .L4a0f78
.L4a0f58:
 mov    eax,DWORD PTR [ecx]
.L4a0f5a:
 push   0x1
.L4a0f5c:
 push   ecx
.L4a0f5d:
 call   DWORD PTR [eax+0x18]
.L4a0f60:
 mov    eax,DWORD PTR [esi+0x488]
.L4a0f66:
 push   0x1
.L4a0f68:
 lea    eax,[eax+eax*2]
.L4a0f6b:
 mov    eax,DWORD PTR [esi+eax*4+0x478]
.L4a0f72:
 push   eax
.L4a0f73:
 mov    ecx,DWORD PTR [eax]
.L4a0f75:
 call   DWORD PTR [ecx+0x18]
.L4a0f78:
 cmp    BYTE PTR [edi+0x98],0x0
.L4a0f7f:
 jne .L4a0f8a
.L4a0f81:
 cmp    BYTE PTR [edi+0x99],0x0
.L4a0f88:
 je .L4a0f96
.L4a0f8a:
 mov    BYTE PTR [esi],0x1
.L4a0f8d:
 mov    al,BYTE PTR [edi+0x99]
.L4a0f93:
 mov    BYTE PTR [esi+0x2],al
.L4a0f96:
 cmp    BYTE PTR [edi+0x9a],0x0
.L4a0f9d:
 je .L4a0fb2
.L4a0f9f:
 push   DWORD PTR [esi+0x498]
.L4a0fa5:
 {load} mov    ecx,esi
.L4a0fa7:
 push   DWORD PTR [esi+0x494]
.L4a0fad:
 call target_4adf00
.L4a0fb2:
 cmp    BYTE PTR [edi+0x1a4],0x0
.L4a0fb9:
 je .L4a0fc2
.L4a0fbb:
 {load} mov    ecx,esi
.L4a0fbd:
 call target_4b0840
.L4a0fc2:
 cmp    DWORD PTR [esi+0x254],0x0
.L4a0fc9:
 mov    eax,0x1
.L4a0fce:
 mov    DWORD PTR [esp+0x50],eax
.L4a0fd2:
 je .L4a0fe7
.L4a0fd4:
 cmp    DWORD PTR [esi+0x258],0x0
.L4a0fdb:
 mov    ecx,0x2
.L4a0fe0:
 cmovne eax,ecx
.L4a0fe3:
 mov    DWORD PTR [esp+0x50],eax
.L4a0fe7:
 {load} xor    ebx,ebx
.L4a0fe9:
 mov    DWORD PTR [esp+0x4c],0x0
.L4a0ff1:
 mov    DWORD PTR [esp+0x6c],0x0
.L4a0ff9:
 mov    DWORD PTR [esp+0x48],ebx
.L4a0ffd:
 test   eax,eax
.L4a0fff:
 je .L4a141e
.L4a1005:
 jmp .L4a1010
.L4a1007:
 {disp32} lea    esp,[esp+0x0]
.L4a100e:
 {load} mov    edi,edi
.L4a1010:
 lea    eax,[esp+0xb0]
.L4a1017:
 push   eax
.L4a1018:
 call   DWORD PTR [0x92d0f4]
.L4a101e:
 lea    eax,[esp+0xa8]
.L4a1025:
 push   eax
.L4a1026:
 call   DWORD PTR [0x92d0f4]
.L4a102c:
 mov    ecx,DWORD PTR [edi+0x28]
.L4a102f:
 mov    eax,0x92492493
.L4a1034:
 sub    ecx,DWORD PTR [edi+0x24]
.L4a1037:
 imul   ecx
.L4a1039:
 {load} add    edx,ecx
.L4a103b:
 sar    edx,0x4
.L4a103e:
 {load} mov    eax,edx
.L4a1040:
 shr    eax,0x1f
.L4a1043:
 {load} add    eax,edx
.L4a1045:
 test   ebx,ebx
.L4a1047:
 jne .L4a106a
.L4a1049:
 mov    ecx,DWORD PTR [edi+0x34]
.L4a104c:
 sub    ecx,DWORD PTR [edi+0x30]
.L4a104f:
 mov    DWORD PTR [esp+0x4c],eax
.L4a1053:
 mov    eax,0x2e8ba2e9
.L4a1058:
 imul   ecx
.L4a105a:
 sar    edx,0x3
.L4a105d:
 {load} mov    eax,edx
.L4a105f:
 shr    eax,0x1f
.L4a1062:
 {load} add    eax,edx
.L4a1064:
 mov    DWORD PTR [esp+0x6c],eax
.L4a1068:
 jmp .L4a10e3
.L4a106a:
 mov    edx,DWORD PTR [esp+0x4c]
.L4a106e:
 {load} cmp    eax,edx
.L4a1070:
 jbe .L4a1095
.L4a1072:
 mov    eax,DWORD PTR [edi+0x24]
.L4a1075:
 lea    ecx,[edx*8+0x0]
.L4a107c:
 push   DWORD PTR [edi+0x28]
.L4a107f:
 {load} sub    ecx,edx
.L4a1081:
 lea    eax,[eax+ecx*4]
.L4a1084:
 push   eax
.L4a1085:
 lea    eax,[esp+0x84]
.L4a108c:
 push   eax
.L4a108d:
 lea    ecx,[edi+0x24]
.L4a1090:
 call target_4b2480
.L4a1095:
 mov    ecx,DWORD PTR [edi+0x34]
.L4a1098:
 mov    eax,0x2e8ba2e9
.L4a109d:
 sub    ecx,DWORD PTR [edi+0x30]
.L4a10a0:
 imul   ecx
.L4a10a2:
 sar    edx,0x3
.L4a10a5:
 {load} mov    eax,edx
.L4a10a7:
 shr    eax,0x1f
.L4a10aa:
 {load} add    eax,edx
.L4a10ac:
 mov    edx,DWORD PTR [esp+0x6c]
.L4a10b0:
 {load} cmp    eax,edx
.L4a10b2:
 jbe .L4a10e3
.L4a10b4:
 mov    ecx,DWORD PTR [edi+0x34]
.L4a10b7:
 imul   eax,edx,0x2c
.L4a10ba:
 mov    edx,DWORD PTR [edi+0x30]
.L4a10bd:
 add    eax,DWORD PTR [edi+0x30]
.L4a10c0:
 {load} cmp    eax,edx
.L4a10c2:
 jne .L4a10cd
.L4a10c4:
 {load} cmp    ecx,ecx
.L4a10c6:
 jne .L4a10cd
.L4a10c8:
 mov    DWORD PTR [edi+0x34],edx
.L4a10cb:
 jmp .L4a10e3
.L4a10cd:
 {load} cmp    eax,ecx
.L4a10cf:
 je .L4a10e3
.L4a10d1:
 push   DWORD PTR [esp+0x7c]
.L4a10d5:
 {load} mov    edx,ecx
.L4a10d7:
 push   eax
.L4a10d8:
 call target_4b31e0
.L4a10dd:
 add    esp,0x8
.L4a10e0:
 mov    DWORD PTR [edi+0x34],eax
.L4a10e3:
 mov    eax,DWORD PTR [esi+0x78]
.L4a10e6:
 lea    edx,[esi+0x41c]
.L4a10ec:
 push   edx
.L4a10ed:
 mov    DWORD PTR [edx],0x0
.L4a10f3:
 push   0x0
.L4a10f5:
 mov    ecx,DWORD PTR [eax]
.L4a10f7:
 push   eax
.L4a10f8:
 call   DWORD PTR [ecx+0x98]
.L4a10fe:
 {load} mov    ecx,esi
.L4a1100:
 call target_4a4f30
.L4a1105:
 test   al,al
.L4a1107:
 jne .L4a113b
.L4a1109:
 mov    eax,DWORD PTR [esi+0x450]
.L4a110f:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a1112:
 push   DWORD PTR [esi+eax*4+0x428]
.L4a1119:
 mov    edx,DWORD PTR [ecx]
.L4a111b:
 push   0x0
.L4a111d:
 push   ecx
.L4a111e:
 call   DWORD PTR [edx+0x94]
.L4a1124:
 mov    edx,DWORD PTR [esi+0x430]
.L4a112a:
 test   edx,edx
.L4a112c:
 je .L4a113b
.L4a112e:
 mov    eax,DWORD PTR [esi+0x78]
.L4a1131:
 push   edx
.L4a1132:
 push   eax
.L4a1133:
 mov    ecx,DWORD PTR [eax]
.L4a1135:
 call   DWORD PTR [ecx+0x9c]
.L4a113b:
 cmp    BYTE PTR [esi+0x208],0x0
.L4a1142:
 mov    eax,0x7
.L4a1147:
 mov    edx,0x3
.L4a114c:
 cmovne edx,eax
.L4a114f:
 mov    eax,DWORD PTR [esi+0x78]
.L4a1152:
 push   0x0
.L4a1154:
 mov    ecx,DWORD PTR [eax]
.L4a1156:
 push   ecx
.L4a1157:
 mov    DWORD PTR [esp],0x3f800000
.L4a115e:
 push   DWORD PTR [edi+ebx*4+0x74]
.L4a1162:
 push   edx
.L4a1163:
 push   0x0
.L4a1165:
 push   0x0
.L4a1167:
 push   eax
.L4a1168:
 call   DWORD PTR [ecx+0xac]
.L4a116e:
 mov    eax,DWORD PTR [esi+0x78]
.L4a1171:
 push   eax
.L4a1172:
 mov    ecx,DWORD PTR [eax]
.L4a1174:
 call   DWORD PTR [ecx+0xa4]
.L4a117a:
 push   ebx
.L4a117b:
 {load} mov    ecx,esi
.L4a117d:
 call target_4a7a50
.L4a1182:
 push   0x1
.L4a1184:
 push   ebx
.L4a1185:
 {load} mov    ecx,esi
.L4a1187:
 call target_4a6760
.L4a118c:
 cmp    BYTE PTR [esi+0x1],0x0
.L4a1190:
 mov    eax,DWORD PTR [esi+0x49c]
.L4a1196:
 mov    ecx,DWORD PTR [esi+0x498]
.L4a119c:
 mov    DWORD PTR [esp+0xe4],eax
.L4a11a3:
 mov    eax,DWORD PTR [esi+0x4a0]
.L4a11a9:
 mov    DWORD PTR [esp+0xe8],eax
.L4a11b0:
 mov    eax,DWORD PTR [esi+0x494]
.L4a11b6:
 mov    DWORD PTR [esp+0xec],eax
.L4a11bd:
 mov    DWORD PTR [esp+0xf0],ecx
.L4a11c4:
 mov    DWORD PTR [esp+0xf4],0x0
.L4a11cf:
 mov    DWORD PTR [esp+0xf8],0x3f800000
.L4a11da:
 je .L4a11fe
.L4a11dc:
 cmp    eax,DWORD PTR [esi+0x48c]
.L4a11e2:
 jne .L4a11fe
.L4a11e4:
 cmp    ecx,DWORD PTR [esi+0x490]
.L4a11ea:
 jne .L4a11fe
.L4a11ec:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a11ef:
 lea    eax,[esi+0x1c]
.L4a11f2:
 push   eax
.L4a11f3:
 push   ecx
.L4a11f4:
 mov    edx,DWORD PTR [ecx]
.L4a11f6:
 call   DWORD PTR [edx+0xbc]
.L4a11fc:
 jmp .L4a1212
.L4a11fe:
 mov    eax,DWORD PTR [esi+0x78]
.L4a1201:
 lea    edx,[esp+0xe4]
.L4a1208:
 push   edx
.L4a1209:
 push   eax
.L4a120a:
 mov    ecx,DWORD PTR [eax]
.L4a120c:
 call   DWORD PTR [ecx+0xbc]
.L4a1212:
 {load} xor    ebx,ebx
.L4a1214:
 test   BYTE PTR [edi+0x84],0x10
.L4a121b:
 mov    DWORD PTR [esp+0x64],ebx
.L4a121f:
 je .L4a123e
.L4a1221:
 lea    eax,[esp+0xbc]
.L4a1228:
 push   eax
.L4a1229:
 call target_4b53c0
.L4a122e:
 mov    eax,[0xbfcc08]
.L4a1233:
 mov    ebx,DWORD PTR [esp+0xcc]
.L4a123a:
 mov    DWORD PTR [esp+0x64],eax
.L4a123e:
 lea    eax,[esp+0xa0]
.L4a1245:
 push   eax
.L4a1246:
 call   DWORD PTR [0x92d0f4]
.L4a124c:
 mov    eax,DWORD PTR [esp+0xa0]
.L4a1253:
 sub    eax,DWORD PTR [esp+0xa8]
.L4a125a:
 movd   xmm0,eax
.L4a125e:
 cvtdq2pd xmm0,xmm0
.L4a1262:
 shr    eax,0x1f
.L4a1265:
 addsd  xmm0,QWORD PTR [eax*8+0xa071b0]
.L4a126e:
 divsd  xmm0,QWORD PTR [0xc00b50]
.L4a1276:
 call target_8f9131
.L4a127b:
 movd   xmm0,eax
.L4a127f:
 cvtdq2pd xmm0,xmm0
.L4a1283:
 shr    eax,0x1f
.L4a1286:
 addsd  xmm0,QWORD PTR [eax*8+0xa071b0]
.L4a128f:
 cvtpd2ps xmm0,xmm0
.L4a1293:
 divss  xmm0,DWORD PTR [0xa0680c]
.L4a129b:
 movss  DWORD PTR [esi+0xb98],xmm0
.L4a12a3:
 mov    eax,DWORD PTR [edi+0x84]
.L4a12a9:
 test   eax,eax
.L4a12ab:
 je .L4a12c1
.L4a12ad:
 push   DWORD PTR [esp+0x64]
.L4a12b1:
 {load} mov    ecx,esi
.L4a12b3:
 push   ebx
.L4a12b4:
 mov    ebx,DWORD PTR [esp+0x50]
.L4a12b8:
 push   eax
.L4a12b9:
 push   ebx
.L4a12ba:
 call target_4abe30
.L4a12bf:
 jmp .L4a12c5
.L4a12c1:
 mov    ebx,DWORD PTR [esp+0x48]
.L4a12c5:
 push   0x1
.L4a12c7:
 push   ebx
.L4a12c8:
 {load} mov    ecx,esi
.L4a12ca:
 call target_4ab9e0
.L4a12cf:
 {load} mov    ecx,esi
.L4a12d1:
 call target_4aaa50
.L4a12d6:
 {load} mov    ecx,esi
.L4a12d8:
 call target_4aa2f0
.L4a12dd:
 mov    eax,DWORD PTR [esi+0x78]
.L4a12e0:
 push   eax
.L4a12e1:
 mov    ecx,DWORD PTR [eax]
.L4a12e3:
 call   DWORD PTR [ecx+0xa8]
.L4a12e9:
 cmp    DWORD PTR [esi+0xa6c],0x0
.L4a12f0:
 jne .L4a12fb
.L4a12f2:
 cmp    DWORD PTR [esi+0xa68],0x0
.L4a12f9:
 je .L4a1323
.L4a12fb:
 mov    eax,DWORD PTR [esi+0x78]
.L4a12fe:
 push   0x0
.L4a1300:
 push   0x0
.L4a1302:
 push   0x0
.L4a1304:
 mov    DWORD PTR [esi+0xa68],0x0
.L4a130e:
 mov    DWORD PTR [esi+0xa6c],0x0
.L4a1318:
 mov    ecx,DWORD PTR [eax]
.L4a131a:
 push   0x0
.L4a131c:
 push   eax
.L4a131d:
 call   DWORD PTR [ecx+0x190]
.L4a1323:
 cmp    DWORD PTR [esi+0xa70],0x0
.L4a132a:
 je .L4a1344
.L4a132c:
 mov    eax,DWORD PTR [esi+0x78]
.L4a132f:
 mov    DWORD PTR [esi+0xa70],0x0
.L4a1339:
 push   0x0
.L4a133b:
 push   eax
.L4a133c:
 mov    ecx,DWORD PTR [eax]
.L4a133e:
 call   DWORD PTR [ecx+0x1a0]
.L4a1344:
 cmp    DWORD PTR [esi+0xa64],0x0
.L4a134b:
 je .L4a1369
.L4a134d:
 mov    eax,DWORD PTR [esi+0x78]
.L4a1350:
 mov    DWORD PTR [esi+0xa64],0x0
.L4a135a:
 push   DWORD PTR [esi+0x298]
.L4a1360:
 mov    ecx,DWORD PTR [eax]
.L4a1362:
 push   eax
.L4a1363:
 call   DWORD PTR [ecx+0x15c]
.L4a1369:
 test   ebx,ebx
.L4a136b:
 jne .L4a13a2
.L4a136d:
 mov    eax,DWORD PTR [esi+0x488]
.L4a1373:
 add    eax,0x5f
.L4a1376:
 lea    eax,[eax+eax*2]
.L4a1379:
 mov    ecx,DWORD PTR [esi+eax*4]
.L4a137c:
 test   ecx,ecx
.L4a137e:
 je .L4a1388
.L4a1380:
 mov    eax,DWORD PTR [ecx]
.L4a1382:
 push   0x1
.L4a1384:
 push   ecx
.L4a1385:
 call   DWORD PTR [eax+0x18]
.L4a1388:
 {load} mov    ecx,esi
.L4a138a:
 call target_4a4fb0
.L4a138f:
 mov    eax,DWORD PTR [esi+0x78]
.L4a1392:
 push   0x0
.L4a1394:
 push   0x0
.L4a1396:
 push   0x0
.L4a1398:
 mov    ecx,DWORD PTR [eax]
.L4a139a:
 push   0x0
.L4a139c:
 push   eax
.L4a139d:
 call   DWORD PTR [ecx+0x44]
.L4a13a0:
 jmp .L4a13ef
.L4a13a2:
 mov    eax,DWORD PTR [esi+0x254]
.L4a13a8:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a13ab:
 mov    DWORD PTR [esp+0xdc],eax
.L4a13b2:
 mov    eax,DWORD PTR [esi+0x258]
.L4a13b8:
 mov    DWORD PTR [esp+0xe0],eax
.L4a13bf:
 mov    eax,[0xbfcc28]
.L4a13c4:
 push   0x0
.L4a13c6:
 mov    DWORD PTR [esp+0xdc],0x0
.L4a13d1:
 mov    DWORD PTR [esp+0xd8],0x0
.L4a13dc:
 push   DWORD PTR [eax+0x8]
.L4a13df:
 mov    edx,DWORD PTR [ecx]
.L4a13e1:
 lea    eax,[esp+0xdc]
.L4a13e8:
 push   0x0
.L4a13ea:
 push   eax
.L4a13eb:
 push   ecx
.L4a13ec:
 call   DWORD PTR [edx+0x44]
.L4a13ef:
 inc    ebx
.L4a13f0:
 mov    DWORD PTR [esi+0xa9c],0x0
.L4a13fa:
 mov    DWORD PTR [esi+0xaa0],0x0
.L4a1404:
 mov    eax,DWORD PTR [esi+0xaa4]
.L4a140a:
 mov    DWORD PTR [esi+0xaa8],eax
.L4a1410:
 mov    DWORD PTR [esp+0x48],ebx
.L4a1414:
 cmp    ebx,DWORD PTR [esp+0x50]
.L4a1418:
 jb .L4a1010
.L4a141e:
 mov    eax,DWORD PTR [esi+0x488]
.L4a1424:
 mov    ecx,DWORD PTR [esi+0x4c4]
.L4a142a:
 xor    eax,0x1
.L4a142d:
 inc    ecx
.L4a142e:
 mov    DWORD PTR [esi+0x4c4],ecx
.L4a1434:
 lea    eax,[eax+eax*2]
.L4a1437:
 mov    edx,DWORD PTR [esi+eax*4+0x470]
.L4a143e:
 test   edx,edx
.L4a1440:
 je .L4a1528
.L4a1446:
 cmp    ecx,0x1
.L4a1449:
 jbe .L4a1528
.L4a144f:
 xorps  xmm0,xmm0
.L4a1452:
 lea    ecx,[esp+0x80]
.L4a1459:
 push   0x0
.L4a145b:
 push   0x8
.L4a145d:
 movlpd QWORD PTR [esp+0x88],xmm0
.L4a1466:
 movlpd QWORD PTR [esp+0x90],xmm0
.L4a146f:
 movlpd QWORD PTR [esp+0x78],xmm0
.L4a1475:
 mov    eax,DWORD PTR [edx]
.L4a1477:
 push   ecx
.L4a1478:
 push   edx
.L4a1479:
 call   DWORD PTR [eax+0x1c]
.L4a147c:
 test   eax,eax
.L4a147e:
 jne .L4a1528
.L4a1484:
 mov    eax,DWORD PTR [esi+0x488]
.L4a148a:
 lea    edx,[esp+0x88]
.L4a1491:
 xor    eax,0x1
.L4a1494:
 add    eax,0x5f
.L4a1497:
 push   0x0
.L4a1499:
 push   0x8
.L4a149b:
 push   edx
.L4a149c:
 lea    eax,[eax+eax*2]
.L4a149f:
 mov    eax,DWORD PTR [esi+eax*4]
.L4a14a2:
 push   eax
.L4a14a3:
 mov    ecx,DWORD PTR [eax]
.L4a14a5:
 call   DWORD PTR [ecx+0x1c]
.L4a14a8:
 test   eax,eax
.L4a14aa:
 jne .L4a1528
.L4a14ac:
 mov    eax,DWORD PTR [esi+0x488]
.L4a14b2:
 lea    edx,[esp+0x70]
.L4a14b6:
 xor    eax,0x1
.L4a14b9:
 push   0x0
.L4a14bb:
 push   0x8
.L4a14bd:
 push   edx
.L4a14be:
 lea    eax,[eax+eax*2]
.L4a14c1:
 mov    eax,DWORD PTR [esi+eax*4+0x478]
.L4a14c8:
 push   eax
.L4a14c9:
 mov    ecx,DWORD PTR [eax]
.L4a14cb:
 call   DWORD PTR [ecx+0x1c]
.L4a14ce:
 test   eax,eax
.L4a14d0:
 jne .L4a1528
.L4a14d2:
 mov    ecx,DWORD PTR [esp+0x88]
.L4a14d9:
 sub    ecx,DWORD PTR [esp+0x80]
.L4a14e0:
 mov    edx,DWORD PTR [esp+0x8c]
.L4a14e7:
 sbb    edx,DWORD PTR [esp+0x84]
.L4a14ee:
 call target_8f925e
.L4a14f3:
 mov    edx,DWORD PTR [esp+0x74]
.L4a14f7:
 mov    ecx,DWORD PTR [esp+0x70]
.L4a14fb:
 cvtsd2ss xmm0,xmm0
.L4a14ff:
 movss  DWORD PTR [esp+0x50],xmm0
.L4a1505:
 call target_8f925e
.L4a150a:
 movss  xmm1,DWORD PTR [esp+0x50]
.L4a1510:
 cvtsd2ss xmm0,xmm0
.L4a1514:
 divss  xmm1,xmm0
.L4a1518:
 mulss  xmm1,DWORD PTR [0xa0680c]
.L4a1520:
 movss  DWORD PTR [esi+0xb94],xmm1
.L4a1528:
 lea    eax,[esp+0x98]
.L4a152f:
 push   eax
.L4a1530:
 call   DWORD PTR [0x92d0f4]
.L4a1536:
 mov    edi,DWORD PTR [esp+0x98]
.L4a153d:
 {load} mov    eax,edi
.L4a153f:
 sub    eax,DWORD PTR [esi+0x4c0]
.L4a1545:
 movd   xmm0,eax
.L4a1549:
 cvtdq2pd xmm0,xmm0
.L4a154d:
 shr    eax,0x1f
.L4a1550:
 addsd  xmm0,QWORD PTR [eax*8+0xa071b0]
.L4a1559:
 divsd  xmm0,QWORD PTR [0xc00b50]
.L4a1561:
 call target_8f9131
.L4a1566:
 sub    esp,0x8
.L4a1569:
 movd   xmm0,eax
.L4a156d:
 cvtdq2pd xmm0,xmm0
.L4a1571:
 shr    eax,0x1f
.L4a1574:
 addsd  xmm0,QWORD PTR [eax*8+0xa071b0]
.L4a157d:
 cvtpd2ps xmm2,xmm0
.L4a1581:
 movss  xmm0,DWORD PTR [esi+0x4b4]
.L4a1589:
 divss  xmm2,DWORD PTR [0xa0680c]
.L4a1591:
 movaps xmm1,xmm2
.L4a1594:
 movss  DWORD PTR [esp+0x50],xmm2
.L4a159a:
 divss  xmm1,xmm0
.L4a159e:
 xorps  xmm0,xmm0
.L4a15a1:
 cvtss2sd xmm0,xmm1
.L4a15a5:
 movsd  QWORD PTR [esp+0x58],xmm0
.L4a15ab:
 fld    QWORD PTR [esp+0x58]
.L4a15af:
 fstp   QWORD PTR [esp]
.L4a15b2:
 call target_8f8f3a
.L4a15b7:
 movss  xmm2,DWORD PTR [esi+0x4b4]
.L4a15bf:
 add    esp,0x8
.L4a15c2:
 movss  xmm1,DWORD PTR [esp+0x48]
.L4a15c8:
 fstp   QWORD PTR [esp+0x50]
.L4a15cc:
 movsd  xmm0,QWORD PTR [esp+0x50]
.L4a15d2:
 cvtpd2ps xmm0,xmm0
.L4a15d6:
 mulss  xmm2,xmm0
.L4a15da:
 movss  xmm0,DWORD PTR [esi+0x4b4]
.L4a15e2:
 mulss  xmm0,DWORD PTR [0xa06564]
.L4a15ea:
 movss  DWORD PTR [esp+0x4c],xmm2
.L4a15f0:
 comiss xmm0,xmm1
.L4a15f3:
 jbe .L4a1603
.L4a15f5:
 movss  xmm2,DWORD PTR [esi+0x4b4]
.L4a15fd:
 movss  DWORD PTR [esp+0x4c],xmm2
.L4a1603:
 cmp    BYTE PTR [esi+0xa5c],0x0
.L4a160a:
 movss  xmm3,DWORD PTR [0xa06618]
.L4a1612:
 je .L4a1656
.L4a1614:
 cmp    BYTE PTR [esi+0xa5d],0x0
.L4a161b:
 je .L4a1656
.L4a161d:
 mov    eax,DWORD PTR [esi+0x488]
.L4a1623:
 mov    eax,DWORD PTR [esi+eax*4+0x468]
.L4a162a:
 cmp    BYTE PTR [eax+0x90],0x0
.L4a1631:
 jne .L4a1656
.L4a1633:
 movss  xmm0,DWORD PTR [esi+0x4b4]
.L4a163b:
 mulss  xmm0,xmm3
.L4a163f:
 comiss xmm0,xmm2
.L4a1642:
 jbe .L4a1656
.L4a1644:
 movss  xmm2,DWORD PTR [esi+0x4b4]
.L4a164c:
 mulss  xmm2,xmm3
.L4a1650:
 movss  DWORD PTR [esp+0x4c],xmm2
.L4a1656:
 comiss xmm2,xmm1
.L4a1659:
 jbe .L4a16e6
.L4a165f:
 mov    ebx,DWORD PTR [0x92d0d0]
.L4a1665:
 movaps xmm0,xmm2
.L4a1668:
 subss  xmm0,xmm1
.L4a166c:
 comiss xmm0,xmm3
.L4a166f:
 jbe .L4a1675
.L4a1671:
 push   0x1
.L4a1673:
 call   ebx
.L4a1675:
 lea    eax,[esp+0x58]
.L4a1679:
 push   eax
.L4a167a:
 call   DWORD PTR [0x92d0f4]
.L4a1680:
 mov    edi,DWORD PTR [esp+0x58]
.L4a1684:
 {load} mov    eax,edi
.L4a1686:
 sub    eax,DWORD PTR [esi+0x4c0]
.L4a168c:
 movd   xmm0,eax
.L4a1690:
 cvtdq2pd xmm0,xmm0
.L4a1694:
 shr    eax,0x1f
.L4a1697:
 addsd  xmm0,QWORD PTR [eax*8+0xa071b0]
.L4a16a0:
 divsd  xmm0,QWORD PTR [0xc00b50]
.L4a16a8:
 call target_8f9131
.L4a16ad:
 movss  xmm2,DWORD PTR [esp+0x4c]
.L4a16b3:
 movss  xmm3,DWORD PTR [0xa06618]
.L4a16bb:
 movd   xmm0,eax
.L4a16bf:
 cvtdq2pd xmm0,xmm0
.L4a16c3:
 shr    eax,0x1f
.L4a16c6:
 addsd  xmm0,QWORD PTR [eax*8+0xa071b0]
.L4a16cf:
 cvtpd2ps xmm1,xmm0
.L4a16d3:
 divss  xmm1,DWORD PTR [0xa0680c]
.L4a16db:
 comiss xmm2,xmm1
.L4a16de:
 ja .L4a1665
.L4a16e0:
 movss  DWORD PTR [esp+0x48],xmm1
.L4a16e6:
 movss  xmm1,DWORD PTR [esi+0x4b4]
.L4a16ee:
 movss  xmm0,DWORD PTR [esp+0x48]
.L4a16f4:
 divss  xmm0,xmm1
.L4a16f8:
 call target_8f8f50
.L4a16fd:
 mov    ecx,DWORD PTR [esi+0x488]
.L4a1703:
 movss  xmm1,DWORD PTR [esp+0x48]
.L4a1709:
 mov    DWORD PTR [esi+ecx*4+0x4d0],eax
.L4a1710:
 mov    DWORD PTR [esi+0x4d8],eax
.L4a1716:
 movss  xmm0,DWORD PTR [esi+0x4b4]
.L4a171e:
 comiss xmm1,xmm0
.L4a1721:
 mov    DWORD PTR [esi+0x4c0],edi
.L4a1727:
 movss  DWORD PTR [esi+0xb90],xmm1
.L4a172f:
 jbe .L4a17c5
.L4a1735:
 movss  xmm0,DWORD PTR [esi+0x4b4]
.L4a173d:
 lea    eax,[esp+0x90]
.L4a1744:
 push   eax
.L4a1745:
 movss  DWORD PTR [esp+0x54],xmm0
.L4a174b:
 call   DWORD PTR [0x92d0f0]
.L4a1751:
 movss  xmm2,DWORD PTR [esp+0x48]
.L4a1757:
 cvtps2pd xmm0,xmm2
.L4a175a:
 movsd  QWORD PTR [esp+0x58],xmm0
.L4a1760:
 movss  xmm0,DWORD PTR [esp+0x50]
.L4a1766:
 fld    QWORD PTR [esp+0x58]
.L4a176a:
 cvtps2pd xmm0,xmm0
.L4a176d:
 movsd  QWORD PTR [esp+0x58],xmm0
.L4a1773:
 fld    QWORD PTR [esp+0x58]
.L4a1777:
 call target_8fc1a2
.L4a177c:
 mov    edx,DWORD PTR [esp+0x94]
.L4a1783:
 mov    ecx,DWORD PTR [esp+0x90]
.L4a178a:
 fstp   QWORD PTR [esp+0x58]
.L4a178e:
 movsd  xmm0,QWORD PTR [esp+0x58]
.L4a1794:
 cvtpd2ps xmm0,xmm0
.L4a1798:
 cvtps2pd xmm0,xmm0
.L4a179b:
 movsd  QWORD PTR [esp+0x58],xmm0
.L4a17a1:
 call target_8f92ef
.L4a17a6:
 divsd  xmm0,QWORD PTR [0xa06700]
.L4a17ae:
 mulsd  xmm0,QWORD PTR [esp+0x58]
.L4a17b4:
 call target_8f9131
.L4a17b9:
 sub    DWORD PTR [esi+0x4c0],eax
.L4a17bf:
 movss  xmm1,DWORD PTR [esp+0x48]
.L4a17c5:
 addss  xmm1,DWORD PTR [esi+0xb84]
.L4a17cd:
 mov    eax,DWORD PTR [esi+0xb88]
.L4a17d3:
 movss  xmm2,DWORD PTR [0xa0680c]
.L4a17db:
 inc    eax
.L4a17dc:
 mov    DWORD PTR [esi+0xb88],eax
.L4a17e2:
 comiss xmm1,xmm2
.L4a17e5:
 movss  DWORD PTR [esi+0xb84],xmm1
.L4a17ed:
 jb .L4a181a
.L4a17ef:
 movd   xmm0,eax
.L4a17f3:
 cvtdq2ps xmm0,xmm0
.L4a17f6:
 mov    DWORD PTR [esi+0xb84],0x0
.L4a1800:
 mov    DWORD PTR [esi+0xb88],0x0
.L4a180a:
 divss  xmm1,xmm0
.L4a180e:
 divss  xmm2,xmm1
.L4a1812:
 movss  DWORD PTR [esi+0xb8c],xmm2
.L4a181a:
 mov    eax,DWORD PTR [esi+0x488]
.L4a1820:
 mov    BYTE PTR [eax+esi*1+0x454],0x1
.L4a1828:
 mov    eax,DWORD PTR [esi+0x488]
.L4a182e:
 mov    edx,DWORD PTR [esi+eax*4+0x460]
.L4a1835:
 lea    ecx,[eax+0x1]
.L4a1838:
 cmp    ecx,0x2
.L4a183b:
 push   edx
.L4a183c:
 {load} sbb    eax,eax
.L4a183e:
 {load} and    eax,ecx
.L4a1840:
 mov    DWORD PTR [esi+0x488],eax
.L4a1846:
 call   DWORD PTR [0x92d120]
.L4a184c:
 mov    ecx,DWORD PTR [esp+0xfc]
.L4a1853:
 pop    edi
.L4a1854:
 pop    esi
.L4a1855:
 pop    ebx
.L4a1856:
 {load} xor    ecx,esp
.L4a1858:
 call target_8f8958
.L4a185d:
 {load} mov    esp,ebp
.L4a185f:
 pop    ebp
.L4a1860:
 ret
.equ target_4a1900,0x4a1900
.equ target_4a22e0,0x4a22e0
.equ target_4a4f30,0x4a4f30
.equ target_4a4fb0,0x4a4fb0
.equ target_4a6330,0x4a6330
.equ target_4a6560,0x4a6560
.equ target_4a6760,0x4a6760
.equ target_4a7a50,0x4a7a50
.equ target_4aa2f0,0x4aa2f0
.equ target_4aaa50,0x4aaa50
.equ target_4ab9e0,0x4ab9e0
.equ target_4abe30,0x4abe30
.equ target_4adf00,0x4adf00
.equ target_4b0840,0x4b0840
.equ target_4b2480,0x4b2480
.equ target_4b31e0,0x4b31e0
.equ target_4b53c0,0x4b53c0
.equ target_542f80,0x542f80
.equ target_8f8958,0x8f8958
.equ target_8f8f3a,0x8f8f3a
.equ target_8f8f50,0x8f8f50
.equ target_8f9131,0x8f9131
.equ target_8f925e,0x8f925e
.equ target_8f92ef,0x8f92ef
.equ target_8fc1a2,0x8fc1a2
