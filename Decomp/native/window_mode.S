.intel_syntax noprefix
.code32
.text
.global original_window_mode
.equ cookie_check,0x8f8958
original_window_mode:
 push ebp
 {load} mov ebp,esp
 sub esp,0x50
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,ebp
 mov DWORD PTR [ebp-8],eax
 push ebx
 mov bl,BYTE PTR [ebp+0x10]
 mov eax,0x80000000
 push esi
 push edi
 test bl,bl
 mov esi,0xcf0000
 {load} mov edi,ecx
 cmovne esi,eax
 push esi
 push -16
 push DWORD PTR [edi+4]
 call DWORD PTR [0x92d5ac]
 mov eax,DWORD PTR [ebp+8]
 mov DWORD PTR [ebp-0x10],eax
 mov eax,DWORD PTR [ebp+0xc]
 push 0
 mov DWORD PTR [ebp-0xc],eax
 lea eax,[ebp-0x18]
 push esi
 push eax
 mov DWORD PTR [ebp-0x14],0
 mov DWORD PTR [ebp-0x18],0
 call DWORD PTR [0x92d580]
 test bl,bl
 je .windowed
 mov eax,DWORD PTR [ebp-0xc]
 sub eax,DWORD PTR [ebp-0x14]
 push 0x62
 push eax
 mov eax,DWORD PTR [ebp-0x10]
 sub eax,DWORD PTR [ebp-0x18]
 push eax
 push 0
 push 0
 jmp .position
.windowed:
 mov eax,DWORD PTR [ebp-0x10]
 sub eax,DWORD PTR [ebp-0x18]
 mov esi,DWORD PTR [ebp-0xc]
 sub esi,DWORD PTR [ebp-0x14]
 push 2
 push DWORD PTR [edi+0x10]
 mov DWORD PTR [ebp-0x44],eax
 push DWORD PTR [edi+0xc]
 call DWORD PTR [0x92d520]
 xorps xmm0,xmm0
 lea ecx,[ebp-0x40]
 push ecx
 movdqu XMMWORD PTR [ebp-0x40],xmm0
 push eax
 movdqu XMMWORD PTR [ebp-0x30],xmm0
 mov DWORD PTR [ebp-0x40],0x28
 movq QWORD PTR [ebp-0x20],xmm0
 call DWORD PTR [0x92d58c]
 test eax,eax
 je .windowposition
 mov eax,DWORD PTR [ebp-0x24]
 mov edx,DWORD PTR [ebp-0x2c]
 {load} sub eax,edx
 mov ecx,DWORD PTR [ebp-0x28]
 mov DWORD PTR [ebp-0x48],eax
 mov eax,DWORD PTR [ebp-0x20]
 {load} sub eax,ecx
 mov DWORD PTR [ebp-0x4c],eax
 mov eax,DWORD PTR [edi+0xc]
 {load} sub eax,edx
 add eax,DWORD PTR [ebp-0x44]
 cmp eax,DWORD PTR [ebp-0x48]
 jbe .vertical
 sub edx,DWORD PTR [ebp-0x44]
 mov eax,DWORD PTR [ebp-0x48]
 {load} add eax,edx
 {load} xor edx,edx
 test eax,eax
 cmovs eax,edx
 mov DWORD PTR [edi+0xc],eax
.vertical:
 mov edx,DWORD PTR [ebp-0x4c]
 {load} mov eax,esi
 {load} sub eax,ecx
 add eax,DWORD PTR [edi+0x10]
 {load} cmp eax,edx
 jbe .windowposition
 {load} sub ecx,esi
 lea eax,[ecx+edx*1]
 {load} xor ecx,ecx
 test eax,eax
 cmovs eax,ecx
 mov DWORD PTR [edi+0x10],eax
.windowposition:
 push 0x60
 push esi
 push DWORD PTR [ebp-0x44]
 push DWORD PTR [edi+0x10]
 push DWORD PTR [edi+0xc]
.position:
 push -2
 push DWORD PTR [edi+4]
 call DWORD PTR [0x92d56c]
 mov ecx,DWORD PTR [ebp-8]
 mov BYTE PTR [edi+0x28],bl
 {load} xor ecx,ebp
 pop edi
 pop esi
 pop ebx
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 12
