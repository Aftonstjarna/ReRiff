.intel_syntax noprefix
.code32
.text
.global original_save_device_exists
.equ make_wide,0x44f3e0
.equ join_path,0x536220
.equ cookie_check,0x8f8958
original_save_device_exists:
 push ebp
 {load} mov ebp,esp
 and esp,-8
 push -1
 push 0x906288
 mov eax,DWORD PTR fs:0
 push eax
 sub esp,0x3c
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,esp
 mov DWORD PTR [esp+0x34],eax
 push ebx
 push esi
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,esp
 push eax
 lea eax,[esp+0x48]
 mov DWORD PTR fs:0,eax
 {load} mov esi,ecx
 mov edx,DWORD PTR [ebp+8]
 lea ecx,[esp+0x28]
 call make_wide
 push eax
 lea edx,[esi+0x14]
 mov DWORD PTR [esp+0x54],0
 lea ecx,[esp+0x14]
 call join_path
 mov esi,DWORD PTR [0x92d4b0]
 add esp,4
 cmp DWORD PTR [esp+0x3c],8
 jb .filename
 push DWORD PTR [esp+0x28]
 call esi
 add esp,4
.filename:
 cmp DWORD PTR [esp+0x24],8
 lea eax,[esp+0x10]
 push 0x9ffdf8
 cmovae eax,DWORD PTR [esp+0x14]
 push eax
 call DWORD PTR [0x92d348]
 add esp,8
 test eax,eax
 jne .opened
 {load} xor bl,bl
 jmp .cleanup
.opened:
 push eax
 call DWORD PTR [0x92d470]
 add esp,4
 mov bl,1
.cleanup:
 cmp DWORD PTR [esp+0x24],8
 jb .done
 push DWORD PTR [esp+0x10]
 call esi
 add esp,4
.done:
 {load} mov al,bl
 mov ecx,DWORD PTR [esp+0x48]
 mov DWORD PTR fs:0,ecx
 pop ecx
 pop esi
 pop ebx
 mov ecx,DWORD PTR [esp+0x34]
 {load} xor ecx,esp
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 4
