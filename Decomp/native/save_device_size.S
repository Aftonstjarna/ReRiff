# Drafted mechanically, reviewed against Ghidra and original disassembly.
# Null handle returns zero; otherwise ftell, seek end, ftell, restore position.
# Failed seeks return zero. Original ftell errors are not separately checked.
# Original VA range 0x535c50..0x535cb3
.intel_syntax noprefix
.code32
.text
.global original_save_device_size
original_save_device_size:
.L535c50:
 push   ebp
.L535c51:
 {load} mov    ebp,esp
.L535c53:
 push   ecx
.L535c54:
 push   edi
.L535c55:
 {load} mov    edi,ecx
.L535c57:
 mov    eax,DWORD PTR [edi+0x10]
.L535c5a:
 test   eax,eax
.L535c5c:
 jne .L535c63
.L535c5e:
 pop    edi
.L535c5f:
 {load} mov    esp,ebp
.L535c61:
 pop    ebp
.L535c62:
 ret
.L535c63:
 push   esi
.L535c64:
 mov    esi,DWORD PTR [0x92d3c4]
.L535c6a:
 push   eax
.L535c6b:
 call   esi
.L535c6d:
 push   0x2
.L535c6f:
 push   0x0
.L535c71:
 push   DWORD PTR [edi+0x10]
.L535c74:
 mov    DWORD PTR [ebp-0x4],eax
.L535c77:
 call   DWORD PTR [0x92d3c0]
.L535c7d:
 add    esp,0x10
.L535c80:
 test   eax,eax
.L535c82:
 je .L535c8c
.L535c84:
 pop    esi
.L535c85:
 {load} xor    eax,eax
.L535c87:
 pop    edi
.L535c88:
 {load} mov    esp,ebp
.L535c8a:
 pop    ebp
.L535c8b:
 ret
.L535c8c:
 push   DWORD PTR [edi+0x10]
.L535c8f:
 call   esi
.L535c91:
 push   0x0
.L535c93:
 push   DWORD PTR [ebp-0x4]
.L535c96:
 {load} mov    esi,eax
.L535c98:
 push   DWORD PTR [edi+0x10]
.L535c9b:
 call   DWORD PTR [0x92d3c0]
.L535ca1:
 add    esp,0x10
.L535ca4:
 {load} xor    ecx,ecx
.L535ca6:
 test   eax,eax
.L535ca8:
 cmovne esi,ecx
.L535cab:
 {load} mov    eax,esi
.L535cad:
 pop    esi
.L535cae:
 pop    edi
.L535caf:
 {load} mov    esp,ebp
.L535cb1:
 pop    ebp
.L535cb2:
 ret
