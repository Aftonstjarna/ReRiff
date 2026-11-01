# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x5429a0..0x542a3b
.intel_syntax noprefix
.code32
.text
.global original_buffer_allocate
original_buffer_allocate:
.L5429a0:
 push   ebp
.L5429a1:
 {load} mov    ebp,esp
.L5429a3:
 cmp    BYTE PTR [ebp+0xc],0x0
.L5429a7:
 mov    eax,0x248
.L5429ac:
 push   ebx
.L5429ad:
 push   esi
.L5429ae:
 push   edi
.L5429af:
 mov    edi,DWORD PTR [ebp+0x8]
.L5429b2:
 {load} mov    esi,ecx
.L5429b4:
 push   0x0
.L5429b6:
 mov    ecx,0x228
.L5429bb:
 cmovne ecx,eax
.L5429be:
 mov    eax,DWORD PTR [edi]
.L5429c0:
 lea    ebx,[esi+0x4]
.L5429c3:
 push   ebx
.L5429c4:
 push   0x0
.L5429c6:
 push   0x0
.L5429c8:
 push   ecx
.L5429c9:
 push   DWORD PTR [ebp+0x10]
.L5429cc:
 mov    DWORD PTR [ebp+0xc],ecx
.L5429cf:
 push   edi
.L5429d0:
 call   DWORD PTR [eax+0x68]
.L5429d3:
 test   eax,eax
.L5429d5:
 js .L542a34
.L5429d7:
 mov    ecx,DWORD PTR [edi]
.L5429d9:
 lea    eax,[esi+0x8]
.L5429dc:
 push   0x0
.L5429de:
 push   eax
.L5429df:
 mov    eax,DWORD PTR [ebp+0x14]
.L5429e2:
 push   0x0
.L5429e4:
 push   0x66
.L5429e6:
 push   DWORD PTR [ebp+0xc]
.L5429e9:
 shl    eax,0x2
.L5429ec:
 push   eax
.L5429ed:
 push   edi
.L5429ee:
 call   DWORD PTR [ecx+0x6c]
.L5429f1:
 mov    DWORD PTR [ebp+0xc],eax
.L5429f4:
 test   eax,eax
.L5429f6:
 jns .L542a10
.L5429f8:
 mov    ecx,DWORD PTR [ebx]
.L5429fa:
 push   ecx
.L5429fb:
 mov    edx,DWORD PTR [ecx]
.L5429fd:
 call   DWORD PTR [edx+0x8]
.L542a00:
 mov    eax,DWORD PTR [ebp+0xc]
.L542a03:
 pop    edi
.L542a04:
 pop    esi
.L542a05:
 mov    DWORD PTR [ebx],0x0
.L542a0b:
 pop    ebx
.L542a0c:
 pop    ebp
.L542a0d:
 ret    0x10
.L542a10:
 mov    DWORD PTR [esi],edi
.L542a12:
 mov    eax,DWORD PTR [edi]
.L542a14:
 push   edi
.L542a15:
 call   DWORD PTR [eax+0x4]
.L542a18:
 mov    eax,DWORD PTR [ebp+0x10]
.L542a1b:
 mov    DWORD PTR [esi+0x14],eax
.L542a1e:
 mov    eax,DWORD PTR [ebp+0x14]
.L542a21:
 mov    DWORD PTR [esi+0x18],eax
.L542a24:
 {load} xor    eax,eax
.L542a26:
 mov    DWORD PTR [esi+0xc],0x0
.L542a2d:
 mov    DWORD PTR [esi+0x10],0x0
.L542a34:
 pop    edi
.L542a35:
 pop    esi
.L542a36:
 pop    ebx
.L542a37:
 pop    ebp
.L542a38:
 ret    0x10
