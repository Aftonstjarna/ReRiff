.intel_syntax noprefix
.code32
.text
.global original_save_device_open
.equ make_wide,0x44f3e0
.equ join_path,0x536220
.equ cookie_check,0x8f8958
original_save_device_open:
 push ebp
 {load} mov ebp,esp
 and esp,-8
 push -1
 push 0x906308
 mov eax,DWORD PTR fs:0
 push eax
 sub esp,0x38
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,esp
 mov DWORD PTR [esp+0x30],eax
 push ebx
 push esi
 push edi
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,esp
 push eax
 lea eax,[esp+0x48]
 mov DWORD PTR fs:0,eax
 {load} mov edi,ecx
 mov ebx,DWORD PTR [ebp+12]
 {load} mov eax,ebx
 mov edx,DWORD PTR [ebp+8]
 dec eax
 je .read_mode
 dec eax
 je .update_mode
 dec eax
 je .update_mode
 {load} xor al,al
 jmp .done
.update_mode:
 mov esi,0x9ffe10
 jmp .path
.read_mode:
 mov esi,0x9ffdf8
.path:
 lea ecx,[esp+0x28]
 call make_wide
 push eax
 lea edx,[edi+0x14]
 mov DWORD PTR [esp+0x54],0
 lea ecx,[esp+0x14]
 call join_path
 add esp,4
 cmp DWORD PTR [esp+0x3c],8
 jb .attributes
 push DWORD PTR [esp+0x28]
 call DWORD PTR [0x92d4b0]
 add esp,4
.attributes:
 cmp DWORD PTR [esp+0x24],8
 lea eax,[esp+0x10]
 cmovae eax,DWORD PTR [esp+0x10]
 push eax
 call DWORD PTR [0x92d084]
 cmp eax,-1
 je .failure
 cmp DWORD PTR [esp+0x24],8
 lea eax,[esp+0x10]
 push esi
 cmovae eax,DWORD PTR [esp+0x14]
 push eax
 call DWORD PTR [0x92d348]
 add esp,8
 mov DWORD PTR [edi+0x10],eax
 test eax,eax
 je .failure
 mov DWORD PTR [edi+12],ebx
 mov bl,1
 jmp .free_path
.failure:
 {load} xor bl,bl
.free_path:
 cmp DWORD PTR [esp+0x24],8
 jb .result
 push DWORD PTR [esp+0x10]
 call DWORD PTR [0x92d4b0]
 add esp,4
.result:
 {load} mov al,bl
.done:
 mov ecx,DWORD PTR [esp+0x48]
 mov DWORD PTR fs:0,ecx
 pop ecx
 pop edi
 pop esi
 pop ebx
 mov ecx,DWORD PTR [esp+0x30]
 {load} xor ecx,esp
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 8
