.intel_syntax noprefix
.code32
.text
.global original_window_frame
.equ cookie_check,0x8f8958
original_window_frame:
 push ebp
 {load} mov ebp,esp
 sub esp,0x24
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,ebp
 mov DWORD PTR [ebp-4],eax
 push ebx
 push esi
 push edi
 push 0x9fbdb4
 {load} mov edi,ecx
 call DWORD PTR [0x92d0dc]
 {load} mov esi,eax
 test esi,esi
 je .done
 push 0x9fbd50
 push esi
 call DWORD PTR [0x92d0e0]
 {load} mov ebx,eax
 test ebx,ebx
 je .release
 lea eax,[ebp-0x14]
 push eax
 push DWORD PTR [edi+8]
 call DWORD PTR [0x92d524]
 push 16
 lea eax,[ebp-0x24]
 push eax
 push 9
 push DWORD PTR [edi+8]
 call ebx
 test eax,eax
 js .release
 mov eax,DWORD PTR [ebp-0x10]
 sub eax,DWORD PTR [ebp-0x20]
 test eax,eax
 jle .release
 add DWORD PTR [edi+0x20],eax
.release:
 push esi
 call DWORD PTR [0x92d0e4]
.done:
 mov ecx,DWORD PTR [ebp-4]
 pop edi
 pop esi
 {load} xor ecx,ebp
 pop ebx
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret
