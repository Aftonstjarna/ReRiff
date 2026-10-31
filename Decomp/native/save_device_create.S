.intel_syntax noprefix
.code32
.text
.global original_save_device_create
.equ make_wide,0x44f3e0
.equ join_path,0x536220
.equ substring,0x536070
.equ stack_probe,0x8f9800
.equ memset_thunk,0x8f8d7e
.equ cookie_check,0x8f8958
original_save_device_create:
 push ebp
 {load} mov ebp,esp
 and esp,-8
 push -1
 push 0x9062c6
 mov eax,DWORD PTR fs:0
 push eax
 push ecx
 mov eax,0x1054
 call stack_probe
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,esp
 mov DWORD PTR [esp+0x1050],eax
 push ebx
 push esi
 push edi
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,esp
 push eax
 lea eax,[esp+0x1068]
 mov DWORD PTR fs:0,eax
 {load} mov esi,ecx
 mov edx,DWORD PTR [ebp+8]
 lea ecx,[esp+0x44]
 call make_wide
 push eax
 lea edx,[esi+0x14]
 mov DWORD PTR [esp+0x1074],0
 lea ecx,[esp+0x18]
 call join_path
 add esp,4
 mov BYTE PTR [esp+0x1070],2
 cmp DWORD PTR [esp+0x58],8
 mov ebx,DWORD PTR [0x92d4b0]
 jb .scan_start
 push DWORD PTR [esp+0x44]
 call ebx
 add esp,4
.scan_start:
 mov edi,DWORD PTR [esp+0x24]
 {load} xor eax,eax
 mov esi,2
 mov DWORD PTR [esp+0x58],7
 mov DWORD PTR [esp+0x54],0
 mov WORD PTR [esp+0x44],ax
 {load} cmp edi,esi
 jbe .open
.scan:
 mov ecx,DWORD PTR [esp+0x28]
 lea eax,[esp+0x14]
 mov edx,DWORD PTR [esp+0x14]
 cmp ecx,8
 cmovae eax,edx
 cmp WORD PTR [eax+esi*2],0x5c
 je .directory
 cmp ecx,8
 lea eax,[esp+0x14]
 cmovae eax,edx
 cmp WORD PTR [eax+esi*2],0x2f
 jne .next
.directory:
 lea eax,[esi+1]
 push eax
 push ecx
 lea eax,[esp+0x34]
 push eax
 lea ecx,[esp+0x20]
 call substring
 cmp DWORD PTR [esp+0x40],8
 lea eax,[esp+0x2c]
 cmovae eax,DWORD PTR [esp+0x2c]
 push eax
 call DWORD PTR [0x92d084]
 cmp eax,-1
 jne .free_directory
 cmp DWORD PTR [esp+0x40],8
 lea eax,[esp+0x2c]
 push 0
 cmovae eax,DWORD PTR [esp+0x30]
 push eax
 call DWORD PTR [0x92d088]
 test eax,eax
 je .directory_failure
.free_directory:
 cmp DWORD PTR [esp+0x40],8
 jb .next
 push DWORD PTR [esp+0x2c]
 call ebx
 add esp,4
.next:
 inc esi
 {load} cmp esi,edi
 jb .scan
.open:
 cmp DWORD PTR [esp+0x28],8
 lea eax,[esp+0x14]
 push 0x9ffe18
 cmovae eax,DWORD PTR [esp+0x18]
 push eax
 call DWORD PTR [0x92d348]
 {load} mov ebx,eax
 add esp,8
 test ebx,ebx
 je .failure
 mov edi,DWORD PTR [ebp+12]
 lea eax,[esp+0x5c]
 push 0x1000
 push 0
 push eax
 call memset_thunk
 add esp,12
 test edi,edi
 je .close_success
 mov eax,0x1000
.write:
 cmp edi,0x1000
 {load} mov esi,edi
 push ebx
 cmova esi,eax
 lea eax,[esp+0x60]
 push esi
 push 1
 push eax
 call DWORD PTR [0x92d454]
 add esp,16
 {load} cmp eax,esi
 jne .write_failure
 {load} sub edi,eax
 mov eax,0x1000
 jne .write
.close_success:
 push ebx
 call DWORD PTR [0x92d470]
 add esp,4
 mov bl,1
 jmp .cleanup
.directory_failure:
 cmp DWORD PTR [esp+0x40],8
 jb .failure
 push DWORD PTR [esp+0x2c]
 call ebx
.pop_failure:
 add esp,4
.failure:
 {load} xor bl,bl
.cleanup:
 cmp DWORD PTR [esp+0x28],8
 jb .done
 push DWORD PTR [esp+0x14]
 call DWORD PTR [0x92d4b0]
 add esp,4
.done:
 {load} mov al,bl
 mov ecx,DWORD PTR [esp+0x1068]
 mov DWORD PTR fs:0,ecx
 pop ecx
 pop edi
 pop esi
 pop ebx
 mov ecx,DWORD PTR [esp+0x1050]
 {load} xor ecx,esp
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 8
.write_failure:
 push ebx
 call DWORD PTR [0x92d470]
 jmp .pop_failure
