.intel_syntax noprefix
.code32
.text
.global original_worker_request
# VA 004A1870..004A18F7. Synchronous request handoff to main loop.
original_worker_request:
 push ebp
 {load} mov ebp,esp
 push esi
 {load} mov esi,ecx
 cmp BYTE PTR [esi+0xba0],0
 je .active
 {load} xor al,al
 pop esi
 pop ebp
 ret 4
.active:
 push -1
 push 1
 lea eax,[esi+0x460]
 push eax
 push 2
 call DWORD PTR [0x92d0c8]
 mov eax,DWORD PTR [ebp+8]
 mov DWORD PTR [esi+0xba4],eax
 mov eax,DWORD PTR [esi+0x488]
 mov BYTE PTR [esi+0xba8],0
 mov BYTE PTR [esi+0x50],1
 push DWORD PTR [esi+eax*4+0x460]
 call DWORD PTR [0x92d104]
 mov eax,DWORD PTR [esi+0x488]
 push DWORD PTR [esi+eax*4+0x458]
 call DWORD PTR [0x92d120]
 mov eax,DWORD PTR [esi+0x488]
 push -1
 push DWORD PTR [esi+eax*4+0x460]
 call DWORD PTR [0x92d124]
 mov al,BYTE PTR [esi+0xba8]
 mov BYTE PTR [esi+0x50],0
 pop esi
 pop ebp
 ret 4
