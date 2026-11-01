# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a4d20..0x4a4d73
.intel_syntax noprefix
.code32
.text
.global original_query_release
original_query_release:
.L4a4d20:
 push   esi
.L4a4d21:
 push   edi
.L4a4d22:
 lea    esi,[ecx+0x474]
.L4a4d28:
 mov    edi,0x2
.L4a4d2d:
 {disp8} lea    ecx,[ecx+0x0]
.L4a4d30:
 mov    ecx,DWORD PTR [esi+0x4]
.L4a4d33:
 test   ecx,ecx
.L4a4d35:
 je .L4a4d3d
.L4a4d37:
 mov    eax,DWORD PTR [ecx]
.L4a4d39:
 push   ecx
.L4a4d3a:
 call   DWORD PTR [eax+0x8]
.L4a4d3d:
 mov    ecx,DWORD PTR [esi]
.L4a4d3f:
 mov    DWORD PTR [esi+0x4],0x0
.L4a4d46:
 test   ecx,ecx
.L4a4d48:
 je .L4a4d50
.L4a4d4a:
 mov    eax,DWORD PTR [ecx]
.L4a4d4c:
 push   ecx
.L4a4d4d:
 call   DWORD PTR [eax+0x8]
.L4a4d50:
 mov    ecx,DWORD PTR [esi-0x4]
.L4a4d53:
 mov    DWORD PTR [esi],0x0
.L4a4d59:
 test   ecx,ecx
.L4a4d5b:
 je .L4a4d63
.L4a4d5d:
 mov    eax,DWORD PTR [ecx]
.L4a4d5f:
 push   ecx
.L4a4d60:
 call   DWORD PTR [eax+0x8]
.L4a4d63:
 mov    DWORD PTR [esi-0x4],0x0
.L4a4d6a:
 add    esi,0xc
.L4a4d6d:
 dec    edi
.L4a4d6e:
 jne .L4a4d30
.L4a4d70:
 pop    edi
.L4a4d71:
 pop    esi
.L4a4d72:
 ret
