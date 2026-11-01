.intel_syntax noprefix
.code32
.section .worker_delete,"ax"
.global original_worker_delete
original_worker_delete:
 push ebp
 {load} mov ebp,esp
 push esi
 {load} mov esi,ecx
 push DWORD PTR [esi+8]
 mov DWORD PTR [esi],0x9fbe20
 call DWORD PTR [0x92d0d4]
 test BYTE PTR [ebp+8],1
 je .keep
 push esi
 call DWORD PTR [0x92d4b0]
 add esp,4
.keep:
 {load} mov eax,esi
 pop esi
 pop ebp
 ret 4
.section .worker_destroy,"ax"
 push DWORD PTR [ecx+8]
 mov DWORD PTR [ecx],0x9fbe20
 call DWORD PTR [0x92d0d4]
 ret
.section .worker_start,"ax"
 push ebp
 {load} mov ebp,esp
 push esi
 {load} mov esi,ecx
 mov ecx,4
 lea eax,[esi+12]
 push eax
 {load} xor eax,eax
 cmp BYTE PTR [ebp+8],al
 cmovne eax,ecx
 push eax
 push esi
 push 0x48dc50
 push 0
 push 0
 call DWORD PTR [0x92d37c]
 {load} mov edx,eax
 add esp,24
 mov DWORD PTR [esi+8],edx
 cmp edx,-1
 jne .priority
 {load} xor al,al
 pop esi
 pop ebp
 ret 4
.priority:
 mov eax,DWORD PTR [esi+4]
 {load} xor ecx,ecx
 {load} sub eax,ecx
 je .low
 dec eax
 je .normal
 dec eax
 jne .set
 mov ecx,1
 push ecx
 push edx
 call DWORD PTR [0x92d0d8]
 mov al,1
 pop esi
 pop ebp
 ret 4
.normal:
 {load} xor ecx,ecx
 push ecx
 push edx
 call DWORD PTR [0x92d0d8]
 mov al,1
 pop esi
 pop ebp
 ret 4
.low:
 or ecx,-1
.set:
 push ecx
 push edx
 call DWORD PTR [0x92d0d8]
 mov al,1
 pop esi
 pop ebp
 ret 4
.section .worker_dispatch,"ax"
 push ebp
 {load} mov ebp,esp
 mov ecx,DWORD PTR [ebp+8]
 mov eax,DWORD PTR [ecx]
 call DWORD PTR [eax+8]
 pop ebp
 ret 4
