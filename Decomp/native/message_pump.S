.intel_syntax noprefix
.code32
.text
.global original_message_pump
# Original VA 0048CE10..0048CE65. Import addresses remain original.
original_message_pump:
 push ebp
 {load} mov ebp,esp
 sub esp,0x1c
 lea eax,[ebp-0x1c]
 push esi
 mov esi,DWORD PTR [0x92d55c]
 push 1
 push 0
 push 0
 push 0
 push eax
 call esi
 test eax,eax
 je .done
 push ebx
 mov ebx,DWORD PTR [0x92d57c]
 push edi
 mov edi,DWORD PTR [0x92d59c]
 {disp8} lea ecx,[ecx+0]
.loop:
 lea eax,[ebp-0x1c]
 push eax
 call edi
 lea eax,[ebp-0x1c]
 push eax
 call ebx
 push 1
 push 0
 push 0
 push 0
 lea eax,[ebp-0x1c]
 push eax
 call esi
 test eax,eax
 jne .loop
 pop edi
 pop ebx
.done:
 pop esi
 {load} mov esp,ebp
 pop ebp
 ret
