# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a4c50..0x4a4d19
.intel_syntax noprefix
.code32
.text
.global original_query_create
original_query_create:
.L4a4c50:
 push   ebp
.L4a4c51:
 {load} mov    ebp,esp
.L4a4c53:
 push   ecx
.L4a4c54:
 {load} mov    eax,ecx
.L4a4c56:
 push   ebx
.L4a4c57:
 push   esi
.L4a4c58:
 push   edi
.L4a4c59:
 lea    esi,[eax+0x474]
.L4a4c5f:
 mov    DWORD PTR [ebp-0x4],eax
.L4a4c62:
 {load} xor    ebx,ebx
.L4a4c64:
 {load} mov    edi,esi
.L4a4c66:
 mov    edx,DWORD PTR [eax+0x78]
.L4a4c69:
 lea    eax,[edi-0x4]
.L4a4c6c:
 push   eax
.L4a4c6d:
 push   0xa
.L4a4c6f:
 push   edx
.L4a4c70:
 mov    ecx,DWORD PTR [edx]
.L4a4c72:
 call   DWORD PTR [ecx+0x1d8]
.L4a4c78:
 test   eax,eax
.L4a4c7a:
 js .L4a4ccd
.L4a4c7c:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a4c7f:
 push   edi
.L4a4c80:
 push   0xa
.L4a4c82:
 mov    eax,DWORD PTR [eax+0x78]
.L4a4c85:
 push   eax
.L4a4c86:
 mov    ecx,DWORD PTR [eax]
.L4a4c88:
 call   DWORD PTR [ecx+0x1d8]
.L4a4c8e:
 test   eax,eax
.L4a4c90:
 js .L4a4cbe
.L4a4c92:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a4c95:
 mov    ecx,DWORD PTR [eax+0x78]
.L4a4c98:
 lea    eax,[edi+0x4]
.L4a4c9b:
 push   eax
.L4a4c9c:
 push   0xc
.L4a4c9e:
 push   ecx
.L4a4c9f:
 mov    edx,DWORD PTR [ecx]
.L4a4ca1:
 call   DWORD PTR [edx+0x1d8]
.L4a4ca7:
 test   eax,eax
.L4a4ca9:
 js .L4a4cbe
.L4a4cab:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a4cae:
 inc    ebx
.L4a4caf:
 add    edi,0xc
.L4a4cb2:
 cmp    ebx,0x2
.L4a4cb5:
 jb .L4a4c66
.L4a4cb7:
 pop    edi
.L4a4cb8:
 pop    esi
.L4a4cb9:
 pop    ebx
.L4a4cba:
 {load} mov    esp,ebp
.L4a4cbc:
 pop    ebp
.L4a4cbd:
 ret
.L4a4cbe:
 mov    ecx,DWORD PTR [ebp-0x4]
.L4a4cc1:
 call target_4a4d20
.L4a4cc6:
 pop    edi
.L4a4cc7:
 pop    esi
.L4a4cc8:
 pop    ebx
.L4a4cc9:
 {load} mov    esp,ebp
.L4a4ccb:
 pop    ebp
.L4a4ccc:
 ret
.L4a4ccd:
 mov    edi,0x2
.L4a4cd2:
 mov    ecx,DWORD PTR [esi+0x4]
.L4a4cd5:
 test   ecx,ecx
.L4a4cd7:
 je .L4a4cdf
.L4a4cd9:
 mov    eax,DWORD PTR [ecx]
.L4a4cdb:
 push   ecx
.L4a4cdc:
 call   DWORD PTR [eax+0x8]
.L4a4cdf:
 mov    ecx,DWORD PTR [esi]
.L4a4ce1:
 mov    DWORD PTR [esi+0x4],0x0
.L4a4ce8:
 test   ecx,ecx
.L4a4cea:
 je .L4a4cf2
.L4a4cec:
 mov    eax,DWORD PTR [ecx]
.L4a4cee:
 push   ecx
.L4a4cef:
 call   DWORD PTR [eax+0x8]
.L4a4cf2:
 mov    ecx,DWORD PTR [esi-0x4]
.L4a4cf5:
 mov    DWORD PTR [esi],0x0
.L4a4cfb:
 test   ecx,ecx
.L4a4cfd:
 je .L4a4d05
.L4a4cff:
 mov    eax,DWORD PTR [ecx]
.L4a4d01:
 push   ecx
.L4a4d02:
 call   DWORD PTR [eax+0x8]
.L4a4d05:
 mov    DWORD PTR [esi-0x4],0x0
.L4a4d0c:
 add    esi,0xc
.L4a4d0f:
 dec    edi
.L4a4d10:
 jne .L4a4cd2
.L4a4d12:
 pop    edi
.L4a4d13:
 pop    esi
.L4a4d14:
 pop    ebx
.L4a4d15:
 {load} mov    esp,ebp
.L4a4d17:
 pop    ebp
.L4a4d18:
 ret
.equ target_4a4d20,0x4a4d20
